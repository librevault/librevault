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
#include "directory/DirectoryExchanger.h"

namespace librevault {

fs::path Session::default_config_path(){
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

Session::Session(fs::path glob_config_path) : config_path_(std::move(glob_config_path)) {
	init_log();
	init_config();

	exchanger_ = std::make_unique<DirectoryExchanger>(*this);
}

Session::~Session() {
	exchanger_.reset();	// Deleted explicitly, because it must be deleted before writing config and destroying io_service;

	fs::ofstream options_fs(this->config_path_ / "librevault.conf", std::ios::trunc | std::ios::binary);
	boost::property_tree::info_parser::write_info(options_fs, config_);
}

void Session::init_log() {
	static std::mutex log_mtx;
	std::lock_guard<std::mutex> lk(log_mtx);
	log_ = spdlog::get("Librevault");
	if(!log_){
		spdlog::set_async_mode(1024*1024);

		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(std::make_shared<spdlog::sinks::stderr_sink_mt>());
		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>((config_path_ / "librevault").native(), ".log", 5*1024*1024, 6));

		log_ = std::make_shared<spdlog::logger>("Librevault", sinks.begin(), sinks.end());
		spdlog::register_logger(log_);
		//log_->set_level(spdlog::level::trace);
		spdlog::set_level(spdlog::level::trace);
	}
}

void Session::init_config() {
	bool config_path_created = fs::create_directories(config_path_);
	log_->info() << "Configuration directory: " << this->config_path_ << (config_path_created ? " created" : "");

	fs::path conf_file = config_path_ / "librevault.conf";

	fs::ifstream options_fs(conf_file, std::ios::binary);
	if(!options_fs){
		log_->info() << "Writing default configuration to: " << conf_file;
		// TODO: Create default configuration file
	}

	boost::property_tree::info_parser::read_info(options_fs, config_);
	log_->info() << "Configuration file loaded: " << conf_file;
}

void Session::run(){
	boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);
	signals.async_wait(std::bind(&Session::shutdown, this));

	auto thread_count = config_.get("threads", std::thread::hardware_concurrency());
	if(thread_count <= 0) thread_count = 1;

	log_->info() << "Worker threads: " << thread_count;

	io_service::work work_lock(io_service_);
	std::vector<std::thread> worker_threads;
	for(unsigned i = 2; i <= thread_count; i++){
		worker_threads.emplace_back(std::bind(&Session::run_worker, this, i));	// Running io_service in threads
	}
	run_worker(1);	// Running in main thread. Can be stopped on shutdown() or restart();
	for(auto& thread : worker_threads){
		if(thread.joinable()) thread.join();
	}

	worker_threads.clear();
	io_service_.reset();
}

void Session::run_worker(unsigned worker_number){
	log_->debug() << "Worker #" << worker_number << " started";
	io_service_.run();
	log_->debug() << "Worker #" << worker_number << " stopped";
}

void Session::shutdown(){
	log_->info() << "Exiting...";
	io_service_.stop();
}

} /* namespace librevault */
