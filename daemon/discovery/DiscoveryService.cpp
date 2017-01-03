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
#include "DiscoveryService.h"
#include "control/StateCollector.h"
#include "discovery/bttracker/BTTrackerDiscovery.h"
#include "discovery/multicast/MulticastProvider.h"
#include "discovery/mldht/MLDHTDiscovery.h"
#include "folder/FolderGroup.h"
#include "util/log.h"

namespace librevault {

DiscoveryService::DiscoveryService(NodeKey& node_key, PortMappingService& port_mapping, StateCollector& state_collector, QObject* parent) : QObject(parent), io_service_("DiscoveryService") {
	LOGFUNC();
	multicast_ = new MulticastProvider(this);
	bttracker_ = std::make_unique<BTTrackerDiscovery>(*this, io_service_.ios(), node_key, port_mapping);
	mldht_ = std::make_unique<MLDHTDiscovery>(*this, io_service_.ios(), port_mapping, state_collector);
	LOGFUNCEND();
}

DiscoveryService::~DiscoveryService() {
	LOGFUNC();
	mldht_.reset();
	bttracker_.reset();

	stop();
	LOGFUNCEND();
}

void DiscoveryService::register_group(std::shared_ptr<FolderGroup> group_ptr) {
	std::unique_lock<std::mutex> lk(registered_groups_mtx_);

	LOGFUNC();
	registered_groups_.insert(group_ptr);

	bttracker_->register_group(group_ptr);
	mldht_->register_group(group_ptr);
	LOGFUNCEND();
}

void DiscoveryService::unregister_group(std::shared_ptr<FolderGroup> group_ptr) {
	std::unique_lock<std::mutex> lk(registered_groups_mtx_);

	LOGFUNC();
	mldht_->unregister_group(group_ptr);
	bttracker_->unregister_group(group_ptr);

	registered_groups_.erase(group_ptr);
	LOGFUNCEND();
}

void DiscoveryService::consume_discovered_node(DiscoveryResult cred, std::weak_ptr<FolderGroup> group_ptr) {
	std::unique_lock<std::mutex> lk(registered_groups_mtx_);

	std::shared_ptr<FolderGroup> group_ptr_locked = group_ptr.lock();
	if(group_ptr_locked && registered_groups_.count(group_ptr_locked)) {
		emit discovered(cred, group_ptr);
	}
}

} /* namespace librevault */
