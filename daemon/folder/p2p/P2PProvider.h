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
#include "pch.h"
#pragma once
#include "Client.h"
#include "NodeKey.h"
#include "util/network.h"
#include "util/Loggable.h"

namespace librevault {

class WSServer;
class WSClient;

/* Discovery services */
class StaticDiscovery;
class MulticastDiscovery;
class BTTrackerDiscovery;
class MLDHTDiscovery;

/* Port mapping services */
class PortManager;

class P2PProvider : protected Loggable {
	friend class ControlServer;
public:
	P2PProvider(Client& client);
	virtual ~P2PProvider();

	/* Port mapping services */
	PortManager* portmanager() {return portmanager_.get();}

	/* Loopback detection */
	void mark_loopback(const tcp_endpoint& endpoint);
	bool is_loopback(const tcp_endpoint& endpoint);
	bool is_loopback(const blob& pubkey);

	/* Getters */
	const NodeKey& node_key() const {return node_key_;}

	WSServer* ws_server() {return ws_server_.get();}
	WSClient* ws_client() {return ws_client_.get();}

protected:

private:
	Client& client_;
	NodeKey node_key_;

	/* WebSocket sockets */
	std::unique_ptr<WSServer> ws_server_;
	std::unique_ptr<WSClient> ws_client_;

	/* Port mapping services */
	std::unique_ptr<PortManager> portmanager_;

	/* Discovery services */
	std::unique_ptr<StaticDiscovery> static_discovery_;
	std::unique_ptr<MulticastDiscovery> multicast4_, multicast6_;
	std::unique_ptr<BTTrackerDiscovery> bttracker_;
	std::unique_ptr<MLDHTDiscovery> mldht_;

	/* Loopback detection */
	std::set<tcp_endpoint> loopback_blacklist_;
	std::mutex loopback_blacklist_mtx_;
};

} /* namespace librevault */
