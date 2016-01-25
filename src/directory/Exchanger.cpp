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
#include "Exchanger.h"

#include "../Client.h"
#include "fs/FSDirectory.h"
#include "p2p/P2PProvider.h"
#include "../nat/NATPMPService.h"
#include "../discovery/StaticDiscovery.h"
#include "../discovery/MulticastDiscovery.h"
#include "../discovery/BTTrackerDiscovery.h"

#include "ExchangeGroup.h"

namespace librevault {

Exchanger::Exchanger(Client& client) : Loggable(client), client_(client) {
	p2p_provider_ = std::make_unique<P2PProvider>(client_, *this);
	//cloud_provider_ = std::make_unique<CloudProvider>(client_, *this);

	natpmp_ = std::make_unique<NATPMPService>(client_, *this);
	natpmp_->port_signal.connect(std::bind(&Exchanger::set_public_port, this, std::placeholders::_1));

	static_discovery_ = std::make_unique<StaticDiscovery>(client_, *this);
	multicast4_ = std::make_unique<MulticastDiscovery4>(client_, *this);
	multicast6_ = std::make_unique<MulticastDiscovery6>(client_, *this);
	bttracker_ = std::make_unique<BTTrackerDiscovery>(client_, *this);

	for(auto& folder_config : client_.config().current.folders) {
		add_directory(folder_config);
	}
}
Exchanger::~Exchanger() {}

void Exchanger::register_group(std::shared_ptr<ExchangeGroup> group_ptr) {
	hash_group_.insert({group_ptr->hash(), group_ptr});

	static_discovery_->register_group(group_ptr);

	multicast4_->register_group(group_ptr);
	multicast6_->register_group(group_ptr);

	bttracker_->register_group(group_ptr);
	log_->debug() << log_tag() << "Group registered: " << group_ptr->secret();
}

void Exchanger::unregister_group(std::shared_ptr<ExchangeGroup> group_ptr) {
	bttracker_->unregister_group(group_ptr);

	multicast4_->unregister_group(group_ptr);
	multicast6_->unregister_group(group_ptr);

	static_discovery_->unregister_group(group_ptr);

	hash_group_.erase(group_ptr->hash());
	log_->debug() << log_tag() << "Group unregistered: " << group_ptr->secret();
}

std::shared_ptr<ExchangeGroup> Exchanger::get_group(const blob& hash){
	auto it = hash_group_.find(hash);
	if(it != hash_group_.end())
		return it->second;
	return nullptr;
}


void Exchanger::set_public_port(uint16_t port) {
	public_port_ = port ? port : p2p_provider_->local_endpoint().port();
}

std::list<std::shared_ptr<ExchangeGroup>> Exchanger::groups() const {
	std::list<std::shared_ptr<ExchangeGroup>> groups_list;
	for(auto group_ptr : hash_group_ | boost::adaptors::map_values)
		groups_list.push_back(group_ptr);
	return groups_list;
}

P2PProvider* Exchanger::p2p_provider() {
	return p2p_provider_.get();
}

void Exchanger::add_directory(const Config::FolderConfig& folder_config) {
	auto dir_ptr = std::make_shared<FSDirectory>(folder_config, client_, *this);
	auto group_ptr = get_group(dir_ptr->secret().get_Hash());
	if(!group_ptr){
		group_ptr = std::make_shared<ExchangeGroup>(client_, *this);
		group_ptr->attach(dir_ptr);
		register_group(group_ptr);
	}else{
		throw std::runtime_error("Multiple directories with same key (or derived from the same key) are not supported now");
	}
}

} /* namespace librevault */
