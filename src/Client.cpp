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

#include "control/ControlServer.h"
#include "directory/fs/FSDirectory.h"
#include "directory/p2p/P2PProvider.h"
#include "discovery/StaticDiscovery.h"
#include "discovery/MulticastDiscovery.h"
#include "discovery/BTTrackerDiscovery.h"
#include "directory/ExchangeGroup.h"

namespace librevault {

Client::Client(std::map<std::string, docopt::value> args) {
	// Initializing paths
	if(args["--data"].isString())
		appdata_path_ = args["--data"].asString();
	else
		appdata_path_ = default_appdata_path();

	fs::create_directories(appdata_path_);

	config_path_ = appdata_path_ / (Version::current().lowercase_name() + ".conf");
	log_path_ = appdata_path_ / (Version::current().lowercase_name() + ".log");
	key_path_ = appdata_path_ / "key.pem";
	cert_path_ = appdata_path_ / "cert.pem";

	// Initializing log
	name_ = "Client";
	switch(args["-v"].asLong()) {
		case 2:     init_log(spdlog::level::trace); break;
		case 1:     init_log(spdlog::level::debug); break;
		default:    init_log(spdlog::level::info);
	}

	// Initializing io_service
	dir_monitor_ios_ = std::make_unique<multi_io_service>(*this, "dir_monitor_ios");
	network_ios_ = std::make_unique<multi_io_service>(*this, "network_ios");
	etc_ios_ = std::make_unique<multi_io_service>(*this, "etc_ios");

	// Initializing components
	config_ = std::make_unique<Config>(*this, config_path_);

	p2p_provider_ = std::make_unique<P2PProvider>(*this);
	//cloud_provider_ = std::make_unique<CloudProvider>(*this);

	static_discovery_ = std::make_unique<StaticDiscovery>(*this);
	multicast4_ = std::make_unique<MulticastDiscovery4>(*this);
	multicast6_ = std::make_unique<MulticastDiscovery6>(*this);
	bttracker_ = std::make_unique<BTTrackerDiscovery>(*this);

	control_server_ = std::make_unique<ControlServer>(*this);

	for(auto& folder_config : config().current.folders) {
		add_directory(folder_config);
	}
}

Client::~Client() {
	control_server_.reset();    // Deleted explicitly, because it must be deleted before writing config and destroying io_service;

	bttracker_.reset();
	multicast6_.reset();
	multicast4_.reset();
	static_discovery_.reset();

	p2p_provider_.reset();

	config_.reset();
}

void Client::init_log(spdlog::level::level_enum level) {
	static std::mutex log_mtx;
	std::unique_lock<decltype(log_mtx)> log_lk(log_mtx);
	log_ = spdlog::get(Version::current().name());
	if(!log_){
		spdlog::set_async_mode(1024*1024);

		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(std::make_shared<spdlog::sinks::stderr_sink_mt>());
#if(BOOST_OS_LINUX)
		sinks.push_back(std::make_shared<spdlog::sinks::syslog_sink>(Version::current().name()));
#endif
		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
				(log_path_.parent_path() / log_path_.stem()).generic_string(), // TODO: support filenames with multiple dots
				log_path_.extension().generic_string().substr(1), 5*1024*1024, 6));

		log_ = std::make_shared<spdlog::logger>(Version::current().name(), sinks.begin(), sinks.end());
		spdlog::register_logger(log_);

		log_->set_level(level);
		log_->set_pattern("[%Y-%m-%d %T.%f] [T:%t] [%L] %v");
	}

	cryptodiff::set_logger(log_);

	log_->info() << Version::current().name() << " " << Version::current().version_string();
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

void Client::register_group(std::shared_ptr<ExchangeGroup> group_ptr) {
	hash_group_.insert({group_ptr->hash(), group_ptr});

	static_discovery_->register_group(group_ptr);

	multicast4_->register_group(group_ptr);
	multicast6_->register_group(group_ptr);

	bttracker_->register_group(group_ptr);
	log_->debug() << log_tag() << "Group registered: " << group_ptr->secret();
}

void Client::unregister_group(std::shared_ptr<ExchangeGroup> group_ptr) {
	bttracker_->unregister_group(group_ptr);

	multicast4_->unregister_group(group_ptr);
	multicast6_->unregister_group(group_ptr);

	static_discovery_->unregister_group(group_ptr);

	hash_group_.erase(group_ptr->hash());
	log_->debug() << log_tag() << "Group unregistered: " << group_ptr->secret();
}

std::shared_ptr<ExchangeGroup> Client::get_group(const blob& hash) {
	auto it = hash_group_.find(hash);
	if(it != hash_group_.end())
		return it->second;
	return nullptr;
}

std::list<std::shared_ptr<ExchangeGroup>> Client::groups() const {
	std::list<std::shared_ptr<ExchangeGroup>> groups_list;
	for(auto group_ptr : hash_group_ | boost::adaptors::map_values)
		groups_list.push_back(group_ptr);
	return groups_list;
}

P2PProvider* Client::p2p_provider() {
	return p2p_provider_.get();
}

void Client::add_directory(const Config::FolderConfig& folder_config) {
	auto dir_ptr = std::make_shared<FSDirectory>(folder_config, *this);
	auto group_ptr = get_group(dir_ptr->secret().get_Hash());
	if(!group_ptr) {
		group_ptr = std::make_shared<ExchangeGroup>(*this);
		group_ptr->attach(dir_ptr);
		register_group(group_ptr);
	}else {
		throw std::runtime_error("Multiple directories with same key (or derived from the same key) are not supported now");
	}
}

fs::path Client::default_appdata_path(){
#if BOOST_OS_WINDOWS
	return fs::path(getenv("APPDATA")) / Version::current().name();	//TODO: Change to Proper(tm) WinAPI-ish SHGetKnownFolderPath
#elif BOOST_OS_MACOS
	return fs::path(getenv("HOME")) / "Library/Preferences" / Version::current().name();	// TODO: error-checking
#elif BOOST_OS_LINUX || BOOST_OS_UNIX
	if(char* xdg_ptr = getenv("XDG_CONFIG_HOME"))
		return fs::path(xdg_ptr) / Version::current().name();
	if(char* home_ptr = getenv("HOME"))
		return fs::path(home_ptr) / ".config" / Version::current().name();
	if(char* home_ptr = getpwuid(getuid())->pw_dir)
		return fs::path(home_ptr) / ".config" / Version::current().name();
	return fs::path("/etc/xdg") / Version::current().name();
#else
	// Well, we will add some Android values here. And, maybe, others.
	return fs::path(getenv("HOME")) / Version::current().name();
#endif
}

} /* namespace librevault */
