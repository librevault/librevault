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
#include "../../pch.h"
#pragma once
#include "../RemoteDirectory.h"
#include "P2PProvider.h"

namespace librevault {

class Client;
class ExchangeGroup;
class P2PProvider;
class AbstractParser;

class P2PDirectory : public RemoteDirectory, public std::enable_shared_from_this<P2PDirectory> {
	friend class P2PProvider;
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

	P2PDirectory(Client& client, Exchanger& exchanger, P2PProvider& provider, std::string name, websocketpp::connection_hdl connection_handle);
	P2PDirectory(Client& client, Exchanger& exchanger, P2PProvider& provider, std::string name, websocketpp::connection_hdl connection_handle, std::shared_ptr<ExchangeGroup> exchange_group);
	~P2PDirectory();

	/* Getters */
	const blob& remote_pubkey() const {return remote_pubkey_;}
	void remote_pubkey(blob new_remote_pubkey) {remote_pubkey_ = std::move(new_remote_pubkey);}

	const tcp_endpoint& remote_endpoint() const {return remote_endpoint_;}

	std::string name() const {return name_;}

	blob local_token();
	blob remote_token();

	/* RPC Actions */
	void send_message(const blob& message);

	// Handshake
	void perform_handshake();
	bool is_handshaken() const {return is_handshaken_;}

	// Choking
	void choke();
	void unchoke();
	void interest();
	void uninterest();

	void post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield);
	void post_have_block(const blob& encrypted_data_hash);

	void request_meta(const Meta::PathRevision& revision);
	void post_meta(const Meta::SignedMeta& smeta, const bitfield_type& bitfield);
	void cancel_meta(const Meta::PathRevision& revision);

	void request_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t size);
	void post_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& chunk);
	void cancel_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t size);

protected:
	blob remote_pubkey_;
	void handle_message(const blob& message);

private:
	P2PProvider& provider_;
	P2PProvider::role_type role_;

	tcp_endpoint remote_endpoint_;
	void update_remote_endpoint();

	std::unique_ptr<AbstractParser> parser_;
	bool is_handshaken_ = false;

	websocketpp::connection_hdl connection_handle_;

	/* Message handlers */
	void handle_Handshake(const blob& message_raw);

	void handle_Choke(const blob& message_raw);
	void handle_Unchoke(const blob& message_raw);
	void handle_Interested(const blob& message_raw);
	void handle_NotInterested(const blob& message_raw);

	void handle_HaveMeta(const blob& message_raw);
	void handle_HaveBlock(const blob& message_raw);

	void handle_MetaRequest(const blob& message_raw);
	void handle_MetaReply(const blob& message_raw);
	void handle_MetaCancel(const blob& message_raw);

	void handle_ChunkRequest(const blob& message_raw);
	void handle_ChunkReply(const blob& message_raw);
	void handle_ChunkCancel(const blob& message_raw);
};

} /* namespace librevault */
