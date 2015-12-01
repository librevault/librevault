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
#include "Connection.h"
#include "WireProtocol.pb.h"

namespace librevault {

class Client;
class ExchangeGroup;
class P2PProvider;
class AbstractParser;

class P2PDirectory : public RemoteDirectory, public std::enable_shared_from_this<P2PDirectory> {
public:
	using length_prefix_t = boost::endian::big_uint32_at;

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

	P2PDirectory(std::unique_ptr<Connection>&& connection, Client& client, Exchanger& exchanger, P2PProvider& provider);
	P2PDirectory(std::unique_ptr<Connection>&& connection, std::shared_ptr<ExchangeGroup> exchange_group, Client& client, Exchanger& exchanger, P2PProvider& provider);
	~P2PDirectory();

	const blob& remote_pubkey() const {return connection_->remote_pubkey();}
	const tcp_endpoint& remote_endpoint() const {return connection_->remote_endpoint();}

	std::string name() const {return connection_->remote_string();}

	bool is_handshaken() const {return awaiting_next_ == AWAITING_ANY;}

	blob local_token();
	blob remote_token();

	/* RPC Actions */
	// Choking
	void choke();
	void unchoke();
	void interest();
	void uninterest();

	void post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield);
	void post_have_block(const blob& encrypted_data_hash);

	void request_meta(const Meta::PathRevision& revision);
	void post_meta(const Meta::SignedMeta& smeta);
	void cancel_meta(const Meta::PathRevision& revision);

	void request_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t size);
	void post_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& chunk);
	void cancel_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t size);

private:
	P2PProvider& provider_;
	static std::array<char, 19> pstr;
	const uint8_t version = 1;

	std::unique_ptr<AbstractParser> parser_;

	union protocol_tag {	// 32 bytes, all fields are octets and unaligned.
		struct {
			uint8_t pstrlen;
			std::array<char, 19> pstr;
			uint8_t version;
			std::array<uint8_t, 11> reserved;
		};
		std::array<uint8_t, 32> raw_bytes;
	};

	// Message flow
	enum {AWAITING_PROTOCOL_TAG, AWAITING_HANDSHAKE, AWAITING_ANY} awaiting_next_ = AWAITING_PROTOCOL_TAG;

	std::shared_ptr<blob> receive_buffer_;
	std::unique_ptr<Connection> connection_;

	// Mutexes
	std::mutex disconnect_mtx_;

	void handle_establish(Connection::state state, const boost::system::error_code& error);
	void disconnect(const boost::system::error_code& error = boost::asio::error::no_protocol_option);

	void receive_size();
	void receive_data();

	/* Timeouts */
	boost::asio::steady_timer timeout;
	void bump_timeout();

	/* Message handlers */
	void handle_message();	// Handles message in buffer

	void handle_protocol_tag();
	void handle_Handshake();

	void handle_Choke();
	void handle_Unchoke();
	void handle_Interested();
	void handle_NotInterested();

	void handle_HaveMeta();
	void handle_HaveBlock();

	void handle_MetaRequest();
	void handle_MetaReply();
	void handle_MetaCancel();

	void handle_ChunkRequest();
	void handle_ChunkReply();
	void handle_ChunkCancel();

	/* Message senders */
	void send_protocol_tag();
	void send_Handshake();

	void send_meta_list();
};

} /* namespace librevault */
