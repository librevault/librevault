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
#pragma once
#include "DiscoveryResult.h"
#include "util/blob.h"
#include "util/log_scope.h"
#include "util/multi_io_service.h"
#include "util/network.h"
#include "util/parse_url.h"
#include <QObject>
#include <mutex>
#include <unordered_set>

namespace librevault {

class FolderGroup;
class DiscoveryGroup;

class StaticDiscovery;
class MulticastProvider;
class BTTrackerDiscovery;
class MLDHTDiscovery;

class NodeKey;
class PortMappingService;
class StateCollector;

class DiscoveryService : public QObject {
	Q_OBJECT
	friend class ControlServer;
	LOG_SCOPE("DiscoveryService");
public:
	DiscoveryService(NodeKey& node_key, PortMappingService& port_mapping, StateCollector& state_collector, QObject* parent);
	virtual ~DiscoveryService();

	void run() {io_service_.start(1);}
	void stop() {io_service_.stop();}

	void register_group(std::shared_ptr<FolderGroup> group_ptr);
	void unregister_group(std::shared_ptr<FolderGroup> group_ptr);

	void consume_discovered_node(DiscoveryResult cred, std::weak_ptr<FolderGroup> group_ptr);

signals:
	void discovered(DiscoveryResult result, std::weak_ptr<FolderGroup> group);

protected:
	multi_io_service io_service_;

	MulticastProvider* multicast_;
	std::unique_ptr<BTTrackerDiscovery> bttracker_;
	std::unique_ptr<MLDHTDiscovery> mldht_;

	std::unordered_set<std::shared_ptr<FolderGroup>> registered_groups_;
	std::mutex registered_groups_mtx_;
};

} /* namespace librevault */