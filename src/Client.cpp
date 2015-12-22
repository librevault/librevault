/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Client.h"

#include "directory/Exchanger.h"

namespace librevault {

Client::Client(std::map<std::string, docopt::value> args) {
	// Initializing paths
	if(args["--data"].isString())
		appdata_path_ = args["--data"].asString();
	else
		appdata_path_ = default_appdata_path();
	config_path_ = appdata_path_ / (version().lowercase_name()+".conf");
	log_path_ = appdata_path_ / (version().lowercase_name()+".log");
	key_path_ = appdata_path_ / "key.pem";
	cert_path_ = appdata_path_ / "cert.pem";

	// Initializing log
	switch(args["-v"].asLong()) {
		case 2:     init_log(spdlog::level::trace); break;
		case 1:     init_log(spdlog::level::debug); break;
		default:    init_log(spdlog::level::info);
	}

	// Initializing components
	dir_monitor_ios_ = std::make_unique<multi_io_service>(*this, "dir_monitor_ios");
	network_ios_ = std::make_unique<multi_io_service>(*this, "network_ios");
	etc_ios_ = std::make_unique<multi_io_service>(*this, "etc_ios");

	config_ = std::make_unique<Config>(*this, config_path_);
	exchanger_ = std::make_unique<Exchanger>(*this);
}

Client::~Client() {
	exchanger_.reset();	// Deleted explicitly, because it must be deleted before writing config and destroying io_service;
	config_.reset();
}

void Client::init_log(spdlog::level::level_enum level) {
	static std::mutex log_mtx;
	std::unique_lock<decltype(log_mtx)> log_lk(log_mtx);
	log_ = spdlog::get(version().name());
	if(!log_){
		spdlog::set_async_mode(1024*1024);

		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(std::make_shared<spdlog::sinks::stderr_sink_mt>());
#if(BOOST_OS_LINUX)
		sinks.push_back(std::make_shared<spdlog::sinks::syslog_sink>(version().name()));
#endif
		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
				(log_path_.parent_path() / log_path_.stem()).generic_string(), // TODO: support filenames with multiple dots
				log_path_.extension().generic_string().substr(1), 5*1024*1024, 6));

		log_ = std::make_shared<spdlog::logger>(version().name(), sinks.begin(), sinks.end());
		spdlog::register_logger(log_);

		log_->set_level(level);
		log_->set_pattern("[%Y-%m-%d %T.%f] [T:%t] [%L] %v");
	}

	cryptodiff::set_logger(log_);

	log_->info() << version().name() << " " << version().version_string();
}

void Client::run() {
	dir_monitor_ios_->start(1);
	network_ios_->start(1);
	etc_ios_->start(std::thread::hardware_concurrency());

	// Main loop/signal processing loop
	boost::asio::signal_set signals(main_loop_ios_, SIGINT, SIGTERM);
	signals.async_wait(std::bind(&Client::shutdown, this));

	main_loop_ios_.run();
	main_loop_ios_.reset();
}

void Client::shutdown(){
	log_->info() << "Exiting...";
	dir_monitor_ios_->stop();
	network_ios_->stop();
	etc_ios_->stop();
	main_loop_ios_.stop();
}

fs::path Client::default_appdata_path(){
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
