/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "Client.h"

#include "control/ControlServer.h"
#include "folder/fs/FSFolder.h"
#include "folder/p2p/P2PProvider.h"

#include <nat/PortMappingService.h>
#include <discovery/DiscoveryService.h>

#include <boost/range/adaptor/map.hpp>
#include <util/Loggable.h>

namespace librevault {

Client::Client() {
	// Initializing io_service
	bulk_ios_ = std::make_unique<multi_io_service>("bulk_ios");
	network_ios_ = std::make_unique<multi_io_service>("network_ios");
	etc_ios_ = std::make_unique<multi_io_service>("etc_ios");

	// Initializing components
	node_key_ = std::make_unique<NodeKey>();
	portmanager_ = std::make_unique<PortMappingService>();
	discovery_ = std::make_unique<DiscoveryService>(*this, *node_key_, *portmanager_);
	p2p_provider_ = std::make_unique<P2PProvider>(*this, *node_key_, *portmanager_);

	/* Control Server */
	control_server_ = std::make_unique<ControlServer>(*this);
	control_server_->add_folder_signal.connect(std::bind(&Client::add_folder, this, std::placeholders::_1));
	control_server_->remove_folder_signal.connect(std::bind(&Client::remove_folder, this, std::placeholders::_1));

	for(auto& folder_config : Config::get()->folders()) {
		init_folder(folder_config);
	}

	/* Connecting signals */
	discovery_->discovered_node_signal.connect(std::bind(&P2PProvider::add_node, p2p_provider_.get(), std::placeholders::_1, std::placeholders::_2));
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
	LOGI("Exiting...");

	hash_group_.clear();

	bulk_ios_->stop();
	network_ios_->stop();
	etc_ios_->stop();
	main_loop_ios_.stop();
}

void Client::add_folder(Json::Value json_folder) {
	LOGFUNC();

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
	LOGFUNC();

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
	LOGFUNC();

	auto group_ptr = get_group(params.secret.get_Hash());
	if(!group_ptr) {
		group_ptr = std::make_shared<FolderGroup>(std::move(params), *this);
		hash_group_.insert({group_ptr->hash(), group_ptr});

		folder_added_signal(group_ptr);
	}else
		throw samekey_error();
}

void Client::deinit_folder(const Secret& secret) {
	LOGFUNC();

	auto group_ptr = get_group(secret.get_Hash());

	hash_group_.erase(secret.get_Hash());
	folder_removed_signal(group_ptr);
	LOGD("Group deinitialized: " << secret);
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
