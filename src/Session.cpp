/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Session.h"

#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>
#include <boost/predef.h>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <cryptopp/ecp.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/integer.h>
#include <cryptopp/files.h>

using CryptoPP::ASN1::secp256r1;

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "Directory.h"

namespace librevault {

fs::path Session::get_default_config_path(){
	const char appname[] = "Librevault";

#if BOOST_OS_WINDOWS
	return fs::path(getenv("APPDATA")) / appname;	//TODO: Change to Proper(tm) WinAPI-ish SHGetKnownFolderPath
#elif BOOST_OS_MACOS
	return fs::path(getenv("HOME")) / "Library/Preferences" / appname;	// TODO: error-checking
#elif BOOST_OS_LINUX || BOOST_OS_UNIX
	if(char* xdg_ptr = getenv("XDG_CONFIG_HOME"))
		return fs::path(xdg_ptr) / appname;
	if(char* home_ptr = getenv("HOME"))
		return fs::path(home_ptr) / ".config" / appname;
	if(char* home_ptr = getpwuid(getuid())->pw_dir)
		return fs::path(home_ptr) / ".config" / appname;
	return fs::path("/etc/xdg") / appname;
#else
	// Well, we will add some Android values here. And, maybe, others.
	return fs::path(getenv("HOME")) / appname;
#endif
}

Session::Session(fs::path glob_config_path) : options_path(std::move(glob_config_path)), monitor(ios), log(spdlog::stderr_logger_mt("Session")) {
	spdlog::set_level(spdlog::level::trace);

	bool glob_config_path_created = fs::create_directories(this->options_path);
	log->info() << "Configuration directory: " << this->options_path;

	fs::ifstream options_fs(this->options_path / "librevault.conf", std::ios::binary);
	boost::property_tree::info_parser::read_info(options_fs, options);
	log->info() << "User configuration loaded: " << this->options_path / "librevault.conf";

	node_key = std::make_unique<NodeKey>(this->options_path / "key.pem", this->options_path / "cert.pem");

	cm = std::make_unique<p2p::ConnectionManager>(*this);

	start_directories();
	start_monitor();
}

Session::~Session() {
	fs::ofstream options_fs(this->options_path / "librevault.conf", std::ios::trunc | std::ios::binary);
	boost::property_tree::info_parser::write_info(options_fs, options);
}

void Session::run(){
	work_lock = std::make_unique<io_service::work>(ios);
	boost::asio::signal_set signals(ios, SIGINT, SIGTERM);
	signals.async_wait(std::bind(&Session::shutdown, this));

	auto thread_count = options.get("threads", std::thread::hardware_concurrency());
	if(thread_count <= 0) thread_count = 1;

	log->info() << "Worker threads: " << thread_count;

	for(auto i = 0; i < thread_count-1; i++){
		log->debug() << "Starting thread #" << i+1;
		worker_threads.emplace_back(std::bind((std::size_t(io_service::*)())&io_service::run, &ios));
	}
	ios.run();	// Actually running
	for(auto i = 0; i < thread_count-1; i++){
		log->debug() << "Stopping thread #" << i+1;
		if(worker_threads[i].joinable()) worker_threads[i].join();
	}

	worker_threads.clear();
	ios.reset();
}

void Session::shutdown(){
	log->info() << "Exiting...";
	work_lock.reset();
	ios.stop();
}

void Session::start_directories(){
	auto folder_trees = options.equal_range("folder");
	for(auto folder_tree_it = folder_trees.first; folder_tree_it != folder_trees.second; folder_tree_it++){
		add_directory(folder_tree_it->second);
	}
}

void Session::start_monitor(){
	monitor.async_monitor([this](boost::system::error_code ec, boost::asio::dir_monitor_event ev){
		log->debug() << "[dir_monitor] " << ev;
		fs::path ev_path = fs::absolute(ev.path);
		for(auto it : path_dir){
			auto path_dir_it = it.first.begin();
			auto ev_dir_it = ev_path.begin();
			bool equal = true;
			while(path_dir_it != it.first.end()){
				if(*path_dir_it != *ev_dir_it)	equal = false;
				path_dir_it++; ev_dir_it++;
			}
			if(equal)
				it.second->handle_modification(ev);
		}

		start_monitor();
	});
}

// Directory management
void Session::add_directory(ptree dir_options){
	auto dir_ptr = std::make_shared<Directory>(dir_options, *this);
	hash_dir.insert({dir_ptr->get_key().get_Hash(), dir_ptr});
	path_dir.insert({dir_ptr->get_open_path(), dir_ptr});
	log->info() << "Added directory: k=" << dir_ptr->get_key() << " path=" << dir_ptr->get_open_path();
	signals.directory(dir_ptr, Signals::DIRECTORY_ADDED);
}

void Session::remove_directory(std::shared_ptr<Directory> dir_ptr){
	signals.directory(dir_ptr, Signals::DIRECTORY_REMOVED);
	path_dir.erase(dir_ptr->get_open_path());
	hash_dir.erase(dir_ptr->get_key().get_Hash());
	log->info() << "Removed directory: k=" << dir_ptr->get_key() << " path=" << dir_ptr->get_open_path();
}

} /* namespace librevault */
