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
#include "P2PDirectory.h"
#include "P2PProvider.h"
#include "../../Session.h"

namespace librevault {

/*
 * Pascal-like string specifying protocol name. Based on BitTorrent handshake header.
 * Never thought, that "Librevault" and "BitTorrent" have same length.
*/
std::array<char, 19> P2PDirectory::pstr = {'L', 'i', 'b', 'r', 'e', 'v', 'a', 'u', 'l', 't', ' ', 'P', 'r', 'o', 't', 'o', 'c', 'o', 'l'};
std::string P2PDirectory::user_agent = "Librevault 1.00.0000-g1234567";

P2PDirectory::P2PDirectory(std::unique_ptr<Connection>&& connection, Session& session, Exchanger& exchanger, P2PProvider& provider) :
		AbstractDirectory(session, exchanger), provider_(provider), connection_(std::move(connection)) {
	connection_->establish(std::bind(&P2PDirectory::handle_establish, this, std::placeholders::_1, std::placeholders::_2));
}

P2PDirectory::~P2PDirectory() {}

void P2PDirectory::handle_establish(Connection::state state, const boost::system::error_code& error) {
	if(state == Connection::state::ESTABLISHED){
		if(connection_->get_role() == Connection::role::CLIENT){
			log_->debug() << "Now we will send handshake message";
		}else{
			log_->debug() << "Now we will wait for handshake message";
		}
	}else if(state == Connection::state::CLOSED){
		log_->debug() << "Connection closed";
	}
}

blob P2PDirectory::gen_handshake() {
	Handhsake_1 handshake_packet;

	handshake_packet.pstr = pstr;
	handshake_packet.pstrlen = pstr.size();
	handshake_packet.version = 1;
	handshake_packet.reserved.fill(0);
	std::copy(remote_hash.begin(), remote_hash.end()+handshake_packet.hash.size(), handshake_packet.hash.data());


	blob handshakebuf(pstr.begin(), pstr.end());
	handshakebuf.push_back(version);
}

void P2PDirectory::send_handshake() {
	blob sendbuf(pstr.begin(), pstr.end());

	connection_->send(sendbuf, [this](const boost::system::error_code&, std::size_t){

	});
}

} /* namespace librevault */
