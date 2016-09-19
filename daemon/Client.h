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

#include "control/FolderParams.h"
#include "util/log_scope.h"
#include "util/multi_io_service.h"

#include <boost/signals2/signal.hpp>

namespace librevault {

/* Components */
class ControlServer;
class P2PProvider;

class NodeKey;
class PortMappingService;
class DiscoveryService;
class FolderService;

class Client {
	friend class ControlServer;
public:
	Client();
	virtual ~Client();

	void run();
	void shutdown();

	boost::asio::io_service& ios() {return etc_ios_->ios();}
	boost::asio::io_service& network_ios() {return network_ios_->ios();}
	boost::asio::io_service& bulk_ios() {return bulk_ios_->ios();}
private:
	std::unique_ptr<NodeKey> node_key_;
	std::unique_ptr<PortMappingService> portmanager_;
	std::unique_ptr<DiscoveryService> discovery_;
	std::unique_ptr<FolderService> folder_service_;
	std::unique_ptr<P2PProvider> p2p_provider_;
	std::unique_ptr<ControlServer> control_server_;

	/* Asynchronous/multithreaded operation */
	boost::asio::io_service main_loop_ios_;
	std::unique_ptr<multi_io_service> network_ios_;
	std::unique_ptr<multi_io_service> bulk_ios_;
	std::unique_ptr<multi_io_service> etc_ios_;

	/* Initialization */
	inline const char* log_tag() {return "";}
};

} /* namespace librevault */
