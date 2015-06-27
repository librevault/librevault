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
#include "SyncManager.h"

#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>
#include <boost/predef.h>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "FSDirectory.h"

namespace librevault {

fs::path SyncManager::get_default_config_path(){
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

SyncManager::SyncManager(fs::path glob_config_path) : options_path(std::move(glob_config_path)), monitor(ios) {
	bool glob_config_path_created = fs::create_directories(this->options_path);
	BOOST_LOG_TRIVIAL(debug) << "Configuration directory: " << this->options_path;

	fs::ifstream options_fs(this->options_path / "librevault.conf", std::ios::binary);
	boost::property_tree::info_parser::read_info(options_fs, options);
	BOOST_LOG_TRIVIAL(debug) << "User configuration loaded: " << this->options_path / "librevault.conf";

	init_directories();

	nodedb = std::make_unique<p2p::NodeDB>(ios, signals, options);
	nodedb->start_lsd_announcer();
	nodedb->start_tracker_announcer();
}

SyncManager::~SyncManager() {
	fs::ofstream options_fs(this->options_path / "librevault.conf", std::ios::trunc | std::ios::binary);
	boost::property_tree::info_parser::write_info(options_fs, options);
}

void SyncManager::run(){
	work_lock = std::make_unique<io_service::work>(ios);
	boost::asio::signal_set signals(ios, SIGINT, SIGTERM);
	signals.async_wait(std::bind(&SyncManager::shutdown, this));

	auto thread_count = options.get("threads", std::thread::hardware_concurrency());
	if(thread_count <= 0) thread_count = 1;
	BOOST_LOG_TRIVIAL(debug) << "Worker threads: " << thread_count;

	for(auto i = 0; i < thread_count-1; i++){
		BOOST_LOG_TRIVIAL(debug) << "Starting thread #" << i+1;
		worker_threads.emplace_back(std::bind((std::size_t(io_service::*)())&io_service::run, &ios));
	}
	ios.run();	// Actually running
	for(auto i = 0; i < thread_count-1; i++){
		if(worker_threads[i].joinable()) worker_threads[i].join();
		BOOST_LOG_TRIVIAL(debug) << "Stopped thread #" << i+1;
	}

	worker_threads.clear();
	ios.reset();
}

void SyncManager::shutdown(){
	work_lock.reset();
	ios.stop();
}

void SyncManager::init_directories(){
	auto folder_trees = options.equal_range("folder");
	for(auto folder_tree_it = folder_trees.first; folder_tree_it != folder_trees.second; folder_tree_it++){
		add_directory(folder_tree_it->second);
	}

	BOOST_LOG_TRIVIAL(debug) << "Initializing dir_monitor";
	start_monitor();
}

void SyncManager::add_directory(ptree dir_options){
	auto dir_ptr = std::make_shared<FSDirectory>(dir_options, monitor, ios, signals, options);
	key_dir.insert({dir_ptr->get_key(), dir_ptr});
	path_dir.insert({dir_ptr->get_open_path(), dir_ptr});
	BOOST_LOG_TRIVIAL(info) << "Added directory: k=" << dir_ptr->get_key() << " path=" << dir_ptr->get_open_path();
	signals.directory(dir_ptr, Signals::DIRECTORY_ADDED);
}

void SyncManager::remove_directory(std::shared_ptr<FSDirectory> dir_ptr){
	signals.directory(dir_ptr, Signals::DIRECTORY_REMOVED);
	path_dir.erase(dir_ptr->get_open_path());
	key_dir.erase(dir_ptr->get_key());
	BOOST_LOG_TRIVIAL(info) << "Removed directory: k=" << dir_ptr->get_key() << " path=" << dir_ptr->get_open_path();
}

void SyncManager::start_monitor(){
	monitor.async_monitor([this](boost::system::error_code ec, boost::asio::dir_monitor_event ev){
		BOOST_LOG_TRIVIAL(trace) << ev;
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

} /* namespace librevault */
