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
#include "folder/fs/FSFolder.h"
#include "folder/p2p/P2PProvider.h"

namespace librevault {

Client::Client(std::map<std::string, docopt::value> args) {
	// Initializing log
	name_ = "Client";
	switch(args["-v"].asLong()) {
		case 2:     init_log(spdlog::level::trace); break;
		case 1:     init_log(spdlog::level::debug); break;
		default:    init_log(spdlog::level::info);
	}

	// Initializing io_service
	bulk_ios_ = std::make_unique<multi_io_service>(*this, "bulk_ios");
	network_ios_ = std::make_unique<multi_io_service>(*this, "network_ios");
	etc_ios_ = std::make_unique<multi_io_service>(*this, "etc_ios");

	// Initializing components
	p2p_provider_ = std::make_unique<P2PProvider>(*this);
	//cloud_provider_ = std::make_unique<CloudProvider>(*this);

	/* Control Server */
	control_server_ = std::make_unique<ControlServer>(*this);
	control_server_->add_folder_signal.connect(std::bind(&Client::add_folder, this, std::placeholders::_1));
	control_server_->remove_folder_signal.connect(std::bind(&Client::remove_folder, this, std::placeholders::_1));

	for(auto& folder_config : Config::get()->folders()) {
		init_folder(folder_config);
	}
}

void Client::init_log(spdlog::level::level_enum level) {
	static std::mutex log_mtx;
	std::unique_lock<decltype(log_mtx)> log_lk(log_mtx);
	log_ = spdlog::get(Version::current().name());
	if(!log_){
		//spdlog::set_async_mode(1024);

		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(std::make_shared<spdlog::sinks::stderr_sink_mt>());
#if(BOOST_OS_LINUX)
		sinks.push_back(std::make_shared<spdlog::sinks::syslog_sink>(Version::current().name()));
#endif
		auto& log_path = Config::get()->paths().log_path;
		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
			(log_path.parent_path() / log_path.stem()).native(), // TODO: support filenames with multiple dots
			log_path.extension().native().substr(1), 10 * 1024 * 1024, 9));

		log_ = std::make_shared<spdlog::logger>(Version::current().name(), sinks.begin(), sinks.end());
		spdlog::register_logger(log_);

		log_->set_level(level);
		log_->set_pattern("[%Y-%m-%d %T.%f] [T:%t] [%L] %v");
	}

	log_->info() << Version::current().name() << " " << Version::current().version_string();
}

Client::~Client() {
	control_server_.reset();    // Deleted explicitly, because it must be deleted before writing config and destroying io_service;
	p2p_provider_.reset();
}

void Client::run() {
	bulk_ios_->start(std::thread::hardware_concurrency());
	network_ios_->start(1);
	etc_ios_->start(1);

	// Main loop/signal processing loop
	boost::asio::signal_set signals(main_loop_ios_, SIGINT, SIGTERM);
	signals.async_wait(std::bind(&Client::shutdown, this));

	main_loop_ios_.run();
	main_loop_ios_.reset();
}

void Client::shutdown(){
	log_->info() << "Exiting...";

	hash_group_.clear();

	bulk_ios_->stop();
	network_ios_->stop();
	etc_ios_->stop();
	main_loop_ios_.stop();
}

void Client::add_folder(Json::Value json_folder) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	FolderParams params(json_folder);

	auto folders_copy = Config::get()->folders_custom();

	auto it = std::find_if(folders_copy.begin(), folders_copy.end(), [&](const Json::Value& v){
		return FolderParams(v).secret.get_Hash() == params.secret.get_Hash();
	});
	if(it == folders_copy.end()) {
		folders_copy.append(json_folder);
		Config::get()->set_folders(folders_copy);

		init_folder(params);
	}else
		throw samekey_error();
}

void Client::remove_folder(const Secret& secret) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto folders_copy = Config::get()->folders_custom();
	for(Json::ArrayIndex i = 0; i < folders_copy.size(); i++) {
		if(FolderParams(folders_copy[i]).secret.get_Hash() == secret.get_Hash()) {
			deinit_folder(secret);

			Json::Value temp;
			folders_copy.removeIndex(i, &temp);
			break;
		}
	}

	Config::get()->set_folders(folders_copy);
}

void Client::init_folder(FolderParams params) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto group_ptr = get_group(params.secret.get_Hash());
	if(!group_ptr) {
		group_ptr = std::make_shared<FolderGroup>(std::move(params), *this);
		hash_group_.insert({group_ptr->hash(), group_ptr});

		folder_added_signal(group_ptr);
	}else
		throw samekey_error();
}

void Client::deinit_folder(const Secret& secret) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	auto group_ptr = get_group(secret.get_Hash());

	hash_group_.erase(secret.get_Hash());
	folder_removed_signal(group_ptr);
	log_->debug() << log_tag() << "Group deinitialized: " << secret;
}

std::shared_ptr<FolderGroup> Client::get_group(const blob& hash) {
	auto it = hash_group_.find(hash);
	if(it != hash_group_.end())
		return it->second;
	return nullptr;
}

std::vector<std::shared_ptr<FolderGroup>> Client::groups() const {
	std::vector<std::shared_ptr<FolderGroup>> groups_list;
	for(auto group_ptr : hash_group_ | boost::adaptors::map_values)
		groups_list.push_back(group_ptr);
	return groups_list;
}

P2PProvider* Client::p2p_provider() {
	return p2p_provider_.get();
}

} /* namespace librevault */
