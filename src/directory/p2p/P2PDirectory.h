/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../../pch.h"
#pragma once
#include "../Abstract.h"
#include "Connection.h"
#include "WireProtocol.pb.h"

namespace librevault {

class Session;
class ExchangeGroup;
class P2PProvider;
class AbstractParser;

class P2PDirectory : public AbstractDirectory, public std::enable_shared_from_this<P2PDirectory> {
public:
	using length_prefix_t = boost::endian::big_uint32_t;

	struct error : public std::runtime_error {
		error(const char* what) : std::runtime_error(what){}
	};
	struct protocol_error : public error {
		protocol_error() : error("Protocol error") {}
	};
	struct auth_error : public error {
		auth_error() : error("Remote node couldn't verify its authenticity") {}
	};
	struct directory_not_found : public error {
		directory_not_found() : error("Local directory not found") {}
	};

	P2PDirectory(std::unique_ptr<Connection>&& connection, Session& session, Exchanger& exchanger, P2PProvider& provider);
	P2PDirectory(std::unique_ptr<Connection>&& connection, std::shared_ptr<ExchangeGroup> exchange_group, Session& session, Exchanger& exchanger, P2PProvider& provider);
	~P2PDirectory();

	std::string remote_string() const {return connection_->remote_string();}
	const blob& remote_pubkey() const {return connection_->remote_pubkey();}
	const tcp_endpoint& remote_endpoint() const {return connection_->remote_endpoint();}

	std::string name() const {return remote_string();}

	blob local_token();
	blob remote_token();

	// AbstractDirectory actions
	std::vector<Meta::PathRevision> get_meta_list();

	void post_revision(std::shared_ptr<AbstractDirectory> origin, const Meta::PathRevision& revision);
	void request_meta(std::shared_ptr<AbstractDirectory> origin, const blob& path_id);
	void post_meta(std::shared_ptr<AbstractDirectory> origin, const Meta::SignedMeta& smeta);
	void request_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id);
	void post_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id, const blob& block);

private:
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
	enum {SIZE, DATA} awaiting_receive_next_ = DATA;
	enum {AWAITING_PROTOCOL_TAG, AWAITING_HANDSHAKE, AWAITING_ANY} awaiting_next_ = AWAITING_PROTOCOL_TAG;

	P2PProvider& provider_;
	std::shared_ptr<blob> receive_buffer_;

	std::unique_ptr<Connection> connection_;

	// High-level facilities
	std::weak_ptr<ExchangeGroup> exchange_group_;
	std::map<blob, int64_t> revisions_;

	void attach(const blob& dir_hash);
	void detach();

	void handle_establish(Connection::state state, const boost::system::error_code& error);

	void receive_size();
	void receive_data();
	void handle_message();	// Handles message in buffer

	void send_protocol_tag();
	void send_handshake();
	void send_meta_list();

	void disconnect(const boost::system::error_code& error = boost::asio::error::no_protocol_option);
};

} /* namespace librevault */
