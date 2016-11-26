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
#include "control/Config.h"
#include "control/ControlServer.h"
#include "control/StateCollector.h"
#include "discovery/DiscoveryService.h"
#include "folder/FolderGroup.h"
#include "folder/FolderService.h"
#include "nat/PortMappingService.h"
#include "nodekey/NodeKey.h"
#include "p2p/P2PProvider.h"
#include <util/log.h>

namespace librevault {

Client::Client() {
	// Initializing components
	state_collector_ = std::make_unique<StateCollector>();
	node_key_ = std::make_unique<NodeKey>();
	portmanager_ = std::make_unique<PortMappingService>();
	discovery_ = std::make_unique<DiscoveryService>(*node_key_, *portmanager_, *state_collector_);
	folder_service_ = std::make_unique<FolderService>(*state_collector_);
	p2p_provider_ = std::make_unique<P2PProvider>(*node_key_, *portmanager_, *folder_service_);
	control_server_ = std::make_unique<ControlServer>(*state_collector_);

	/* Connecting signals */
	state_collector_->global_state_changed.connect([this](std::string key, Json::Value value){
		if(control_server_) control_server_->notify_global_state_changed(key, value);
	});
	state_collector_->folder_state_changed.connect([this](const blob& folderid, std::string key, Json::Value value){
		if(control_server_) control_server_->notify_folder_state_changed(folderid, key, value);
	});
	discovery_->discovered_node_signal.connect([this](DiscoveryService::ConnectCredentials node_cred, std::shared_ptr<FolderGroup> group_ptr){
		if(p2p_provider_) p2p_provider_->add_node(node_cred, group_ptr);
	});
	folder_service_->folder_added_signal.connect([this](std::shared_ptr<FolderGroup> group){
		if(discovery_) discovery_->register_group(group);
		if(control_server_) control_server_->notify_folder_added(group->hash(), Config::get()->folder_get(group->hash()));
	});
	folder_service_->folder_removed_signal.connect([this](std::shared_ptr<FolderGroup> group){
		if(discovery_) discovery_->unregister_group(group);
		if(control_server_) control_server_->notify_folder_removed(group->hash());
	});
	control_server_->restart_signal.connect([this]{
		restart();
	});
	control_server_->shutdown_signal.connect([this]{
		shutdown();
	});
}

Client::~Client() {
	control_server_.reset();
	p2p_provider_.reset();
	folder_service_.reset();
	discovery_.reset();
	portmanager_.reset();
	node_key_.reset();
}

void Client::run() {
	portmanager_->run();
	discovery_->run();
	folder_service_->run();
	p2p_provider_->run();
	control_server_->run();

	// Main loop/signal processing loop
	boost::asio::signal_set signals(main_loop_ios_, SIGINT, SIGTERM);
	signals.async_wait(std::bind(&Client::shutdown, this));

	main_loop_ios_.run();
}

void Client::restart() {
	LOGI("Restarting...");
	want_restart_ = true;
	main_loop_ios_.stop();
}

void Client::shutdown(){
	LOGI("Exiting...");
	main_loop_ios_.stop();
}

} /* namespace librevault */
