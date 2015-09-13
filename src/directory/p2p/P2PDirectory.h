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
class FSDirectory;
class P2PProvider;
class P2PDirectory : public AbstractDirectory, public std::enable_shared_from_this<P2PDirectory> {
public:
	P2PDirectory(std::unique_ptr<Connection>&& connection, Session& session, Exchanger& exchanger, P2PProvider& provider);
	P2PDirectory(std::unique_ptr<Connection>&& connection, std::shared_ptr<FSDirectory> directory_ptr, Session& session, Exchanger& exchanger, P2PProvider& provider);
	~P2PDirectory();

	std::string remote_string() const {return connection_->remote_string();}
	blob local_token();
	blob remote_token();

private:
	static std::array<char, 19> pstr;
	const uint8_t version = 1;

	using prefix_t = boost::endian::big_int32_t;

	struct protocol_id {	// 32 bytes, all fields are octets and unaligned.
		uint8_t pstrlen;
		std::array<char, 19> pstr;
		uint8_t version;
		std::array<uint8_t, 11> reserved;
	};

	struct error : public std::runtime_error {
		error(const char* what) : std::runtime_error(what){}
	};

	struct protocol_error : public error {
		protocol_error() : error("Protocol error") {}
	};

	struct directory_not_found : public error {
		directory_not_found() : error("Local directory not found") {}
	};

	enum {SIZE, DATA} awaiting_receive_next_ = DATA;
	enum {AWAITING_PROTOCOL_ID, AWAITING_HANDSHAKE, AWAITING_ANY} awaiting_next_ = AWAITING_PROTOCOL_ID;

	enum message_type : byte {PROTOCOL_ID = 0, HANDSHAKE = 1};

	P2PProvider& provider_;

	std::unique_ptr<Connection> connection_;
	std::weak_ptr<FSDirectory> directory_ptr_;

	std::shared_ptr<blob> receive_buffer_;

	void attach(const blob& dir_hash);
	void detach();

	void handle_establish(Connection::state state, const boost::system::error_code& error);

	void receive_size();
	void receive_data();
	void handle_message();	// Handles message in buffer

	blob generate(message_type type);

	void disconnect(const boost::system::error_code& error = boost::asio::error::no_protocol_option);
};

} /* namespace librevault */
