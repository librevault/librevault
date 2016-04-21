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
#pragma once
#include "folder/RemoteFolder.h"
#include "P2PProvider.h"
#include "WSService.h"
#include "BandwidthCounter.h"
#include <librevault/protocol/V1Parser.h>
#include <websocketpp/common/connection_hdl.hpp>

namespace librevault {

class Client;
class FolderGroup;
class P2PProvider;
class WSService;

class P2PFolder : public RemoteFolder, public std::enable_shared_from_this<P2PFolder> {
	friend class P2PProvider;
	friend class WSService;
	friend class WSServer;
	friend class WSClient;
public:
	/* Errors */
	struct error : public std::runtime_error {
		error(const char* what) : std::runtime_error(what){}
	};
	struct protocol_error : public error {
		protocol_error() : error("Protocol error") {}
	};
	struct auth_error : public error {
		auth_error() : error("Remote node couldn't verify its authenticity") {}
	};

	P2PFolder(Client& client, P2PProvider& provider, WSService& ws_service, WSService::connection conn);
	~P2PFolder();

	/* Getters */
	const blob& remote_pubkey() const {return conn_.remote_pubkey;}
	const tcp_endpoint& remote_endpoint() const {return conn_.remote_endpoint;}
	const WSService::connection::role_type role() const {return conn_.role;}
	const std::string& client_name() const {return client_name_;}
	const std::string& user_agent() const {return user_agent_;}
	std::shared_ptr<FolderGroup> folder_group() const {return std::shared_ptr<FolderGroup>(group_);}
	BandwidthCounter::Stats heartbeat_stats() {return counter_.heartbeat();}

	blob local_token();
	blob remote_token();

	/* RPC Actions */
	void send_message(const blob& message);

	// Handshake
	void perform_handshake();
	bool ready() const {return is_handshaken_;}

	/* Message senders */
	void choke();
	void unchoke();
	void interest();
	void uninterest();

	void post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield);
	void post_have_chunk(const blob& ct_hash);

	void request_meta(const Meta::PathRevision& revision);
	void post_meta(const SignedMeta& smeta, const bitfield_type& bitfield);
	void cancel_meta(const Meta::PathRevision& revision);

	void request_block(const blob& ct_hash, uint32_t offset, uint32_t size);
	void post_block(const blob& ct_hash, uint32_t offset, const blob& block);
	void cancel_block(const blob& ct_hash, uint32_t offset, uint32_t size);

protected:
	WSService::connection conn_;
	std::weak_ptr<FolderGroup> group_;

	void handle_message(const blob& message);

private:
	P2PProvider& provider_;
	WSService& ws_service_;

	V1Parser parser_;   // Protocol parser
	bool is_handshaken_ = false;

	BandwidthCounter counter_;

	// These needed primarily for UI
	std::string client_name_;
	std::string user_agent_;

	/* Message handlers */
	void handle_Handshake(const blob& message_raw);

	void handle_Choke(const blob& message_raw);
	void handle_Unchoke(const blob& message_raw);
	void handle_Interested(const blob& message_raw);
	void handle_NotInterested(const blob& message_raw);

	void handle_HaveMeta(const blob& message_raw);
	void handle_HaveChunk(const blob& message_raw);

	void handle_MetaRequest(const blob& message_raw);
	void handle_MetaReply(const blob& message_raw);
	void handle_MetaCancel(const blob& message_raw);

	void handle_BlockRequest(const blob& message_raw);
	void handle_BlockReply(const blob& message_raw);
	void handle_BlockCancel(const blob& message_raw);
};

} /* namespace librevault */
