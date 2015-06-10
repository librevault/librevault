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
#include <cstdlib>
#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

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

SyncManager::SyncManager() : program_options_desc("Program options"), core_options_desc("Core options"), monitor(ios) {
	// Program options
	program_options_desc.add_options()
		("help,h", "Display help message")
	;

	core_options_desc.add_options()
		("threads,t", po::value<decltype(thread_count)>(&thread_count)->default_value(std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 1), "Number of CPU worker threads. Default value is number of hardware CPU threads")
		("device-name", po::value<std::string>()->default_value(boost::asio::ip::host_name()), "Device name you chosen for this instance of Librevault. Default value is the actual hostname of your computer")
		("config-dir,c", po::value<fs::path>(&global_config_path)->default_value(get_default_config_path()), "Librevault configuration directory. Default value depends on OS ")
	;

	// dir_monitor
	monitor.add_directory("/home/gamepad/syncfs");
	monitor.async_monitor([](boost::system::error_code ec, boost::asio::dir_monitor_event ev){
		BOOST_LOG_TRIVIAL(trace) << ev;
	});
}

SyncManager::SyncManager(int argc, char** argv) : SyncManager() {
	// Configuration
	po::options_description all_options_desc;
	all_options_desc.add(program_options_desc);
	all_options_desc.add(core_options_desc);

	std::cout << all_options_desc;
	po::store(po::parse_command_line(argc, argv, all_options_desc), glob_options);
	po::notify(glob_options);

	// Initialization
	fs::create_directories(glob_options["config-dir"].as<fs::path>());
	BOOST_LOG_TRIVIAL(debug) << "Configuration directory: " << glob_options["config-dir"].as<fs::path>();
}

SyncManager::~SyncManager() {}

/*
ptree SyncManager::get_defaults(){
	ptree defaults;
	defaults.put("core.threads", std::thread::hardware_concurrency());
	defaults.put("core.name", boost::asio::ip::host_name());

	defaults.put("nodedb.udplpdv4.repeat_interval", 30);
	defaults.put("nodedb.udplpdv4.multicast_port", 28914);
	defaults.put("nodedb.udplpdv4.multicast_ip", "239.192.152.144");
	defaults.put("nodedb.udplpdv4.bind_ip", "0.0.0.0");
	defaults.put("nodedb.udplpdv4.bind_port", 28914);

	defaults.put("nodedb.udplpdv6.repeat_interval", 30);
	defaults.put("nodedb.udplpdv6.multicast_port", 28914);
	defaults.put("nodedb.udplpdv6.multicast_ip", "ff08::BD02");
	defaults.put("nodedb.udplpdv6.bind_ip", "0::0");
	defaults.put("nodedb.udplpdv6.bind_port", 28914);

	defaults.put("net.port", 30);
	defaults.put("net.tcp", true);
	defaults.put("net.utp", true);

	return defaults;
}*/

void SyncManager::run(){
	work_lock = std::make_unique<io_service::work>(ios);
	boost::asio::signal_set signals(ios, SIGINT, SIGTERM);
	signals.async_wait(std::bind(&SyncManager::shutdown, this));

	if(thread_count <= 0) thread_count = 1;
	BOOST_LOG_TRIVIAL(debug) << "Max threads: " << thread_count;
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
	for(fs::path path : fs::directory_iterator(global_config_path)){
		if(fs::status(path).type() == fs::regular_file){
			if()
		}
	}
}
void SyncManager::add_directory(ptree dir_options){

}

} /* namespace librevault */
