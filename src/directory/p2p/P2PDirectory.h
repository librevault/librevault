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

namespace librevault {

class Session;
class P2PProvider;
class P2PDirectory : public AbstractDirectory {
public:
	P2PDirectory(std::unique_ptr<Connection>&& connection, Session& session, Exchanger& exchanger, P2PProvider& provider);
	~P2PDirectory();

	void handle_establish(Connection::state state, const boost::system::error_code& error);

private:
	static std::array<char, 19> pstr;
	static std::string user_agent;
	const uint8_t version = 1;

#pragma pack(push, 1)
	struct Handhsake_1 {
		uint8_t pstrlen;
		std::array<char, 19> pstr;
		uint8_t version;
		std::array<uint8_t, 11> reserved;
		std::array<uint8_t, 32> hash;
		std::array<uint8_t, 32> auth_hmac;
	};
#pragma pack(pop)

	P2PProvider& provider_;

	std::unique_ptr<Connection> connection_;
	blob remote_hash;

	blob gen_handshake();
	void send_handshake();
};

} /* namespace librevault */
