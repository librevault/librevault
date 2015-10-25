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

#include "directory/Exchanger.h"

namespace librevault {

Session::Session(const po::variables_map& vm) {
	if(vm.count("config") > 0)
		appdata_path_ = vm["config"].as<fs::path>();
	else
		appdata_path_ = default_appdata_path();
	config_path_ = appdata_path_ / (version().lowercase_name()+".conf");
	log_path_ = appdata_path_ / (version().lowercase_name()+".log");
	key_path_ = appdata_path_ / "key.pem";
	cert_path_ = appdata_path_ / "cert.pem";
	dhparam_path_ = appdata_path_ / "dhparam.pem";

	init_log();
	config_ = std::make_unique<Config>(*this);
	exchanger_ = std::make_unique<Exchanger>(*this);
}

Session::~Session() {
	exchanger_.reset();	// Deleted explicitly, because it must be deleted before writing config and destroying io_service;
	config_.reset();
}

void Session::init_log() {
	static std::mutex log_mtx;
	std::lock_guard<std::mutex> lk(log_mtx);
	log_ = spdlog::get(version().name());
	if(!log_){
		spdlog::set_async_mode(1024*1024);

		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(std::make_shared<spdlog::sinks::stderr_sink_mt>());
		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
				(log_path_.parent_path() / log_path_.stem()).generic_string(), // TODO: support filenames with multiple dots
				log_path_.extension().generic_string().substr(1), 5*1024*1024, 6));

		log_ = std::make_shared<spdlog::logger>(version().name(), sinks.begin(), sinks.end());
		spdlog::register_logger(log_);
		log_->set_level(spdlog::level::trace);
	}

	cryptodiff::set_logger(log_);

	log_->set_pattern("[%Y-%m-%d %T.%e] [T:%t] [%n] [%l] %v");

	log_->info() << version().name() << " " << version().version_string();
}

void Session::run(){
	boost::asio::signal_set signals(io_service_, SIGINT, SIGTERM);
	signals.async_wait(std::bind(&Session::shutdown, this));

	auto thread_count = config().get("threads", std::thread::hardware_concurrency());
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

fs::path Session::default_appdata_path(){
#if BOOST_OS_WINDOWS
	return fs::path(getenv("APPDATA")) / version().name();	//TODO: Change to Proper(tm) WinAPI-ish SHGetKnownFolderPath
#elif BOOST_OS_MACOS
	return fs::path(getenv("HOME")) / "Library/Preferences" / version().name();	// TODO: error-checking
#elif BOOST_OS_LINUX || BOOST_OS_UNIX
	if(char* xdg_ptr = getenv("XDG_CONFIG_HOME"))
		return fs::path(xdg_ptr) / version().name();
	if(char* home_ptr = getenv("HOME"))
		return fs::path(home_ptr) / ".config" / version().name();
	if(char* home_ptr = getpwuid(getuid())->pw_dir)
		return fs::path(home_ptr) / ".config" / version().name();
	return fs::path("/etc/xdg") / version().name();
#else
	// Well, we will add some Android values here. And, maybe, others.
	return fs::path(getenv("HOME")) / version().name();
#endif
}

} /* namespace librevault */
