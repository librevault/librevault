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
#include "../bttracker/UDPTrackerConnection.h"

#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>
#include <boost/lexical_cast.hpp>
#include <cryptopp/osrng.h>
#include "../../htonll.h"

namespace librevault {
namespace p2p {

UDPTrackerConnection::UDPTrackerConnection(url tracker_address, io_service& ios, Signals& signals, ptree& options) :
		TrackerConnection(ios, signals, options), socket(ios), resolver(ios), tracker_address(std::move(tracker_address)), reconnect_timer(ios) {
	assert(this->tracker_address.scheme == "udp");
	if(this->tracker_address.port == 0){this->tracker_address.port = 80;}

	BOOST_LOG_TRIVIAL(debug) << "UDPTracker (" << this->tracker_address.host << "): Added";
}

UDPTrackerConnection::~UDPTrackerConnection() {
	BOOST_LOG_TRIVIAL(debug) << "UDPTracker (" << tracker_address.host << "): Removed";
}

int32_t UDPTrackerConnection::gen_transaction_id() {
	CryptoPP::AutoSeededRandomPool rng;
	int32_t number;
	rng.GenerateBlock(reinterpret_cast<uint8_t*>(&number), sizeof(number));
	return number;
}

void UDPTrackerConnection::start(){
	bind_address = address::from_string(options.get<std::string>("net.ip"));
	socket.open(bind_address.is_v6() ? udp::v6() : udp::v4());
	socket.bind(udp::endpoint(bind_address, 0));

	BOOST_LOG_TRIVIAL(debug) << "==> UDPTracker (" << tracker_address.host << "): Resolving IP address";

	udp::resolver::query resolver_query = udp::resolver::query(tracker_address.host, boost::lexical_cast<std::string>(tracker_address.port));
	resolver.async_resolve(resolver_query, std::bind(&UDPTrackerConnection::handle_resolve, this, std::placeholders::_1, std::placeholders::_2));

	receive_loop();
}

void UDPTrackerConnection::receive_loop(){
	auto buffer_ptr = std::make_shared<udp_buffer>(udb_buffer_max_size);
	auto endpoint_ptr = std::make_shared<udp::endpoint>();
	socket.async_receive_from(boost::asio::buffer((char*)buffer_ptr->data(), buffer_ptr->size()), *endpoint_ptr, std::bind([this](const boost::system::error_code& error, std::size_t bytes_transferred, std::shared_ptr<udp_buffer> buffer_ptr, std::shared_ptr<udp::endpoint> endpoint_ptr){
		if(error == boost::asio::error::operation_aborted) return;
		buffer_ptr->resize(bytes_transferred);

		int32_t transaction_rcvd = *reinterpret_cast<int32_t*>(buffer_ptr->data()+4);
		if(transaction_rcvd != transaction_id) receive_loop(); return;

		Action action_rcvd = (Action)ntohl(*reinterpret_cast<int32_t*>(buffer_ptr->data()));
		if(action_rcvd != action || action_rcvd == ERROR) receive_loop(); return;

		BOOST_LOG_TRIVIAL(debug) << "<== UDPTracker (" << tracker_address.host << ")";

		switch(action_rcvd){
		case CONNECT:
			handle_connect(buffer_ptr);
			break;
		case ANNOUNCE:
		case ANNOUNCE6:
			handle_announce(buffer_ptr);
			break;
		case SCRAPE:
			// Do nothing, seriously.
			break;
		case ERROR:
			handle_error(buffer_ptr);
			break;
		}

		receive_loop();
	}, std::placeholders::_1, std::placeholders::_2, buffer_ptr, endpoint_ptr));
}

void UDPTrackerConnection::handle_resolve(const boost::system::error_code& error, udp::resolver::iterator iterator){
	for(auto& it = iterator; it != udp::resolver::iterator(); it++)
		if(bind_address.is_v6() || iterator->endpoint().address().is_v4())
			target = iterator->endpoint();
	if(target == udp::endpoint()) throw std::runtime_error("Unable to resolve tracker");

	BOOST_LOG_TRIVIAL(debug) << "<== UDPTracker (" << tracker_address.host << "): IP: " << target.address();
	connect();
}

void UDPTrackerConnection::connect(){
	announce_timer.cancel();
	response_timer.cancel();

	conn_req request;
	request.action = CONNECT;
	request.connection_id = htonll(0x41727101980);
	request.transaction_id = gen_transaction_id();

	transaction_id = request.transaction_id;

	socket.async_send_to(boost::asio::buffer((char*)&request, sizeof(request)), target, std::bind([this](int32_t transaction_id){
		BOOST_LOG_TRIVIAL(debug) << "==> UDPTracker (" << tracker_address.host << "): Requesting connection. Transaction ID: " << transaction_id;
	}, transaction_id));
}

void UDPTrackerConnection::announce(const infohash& info_hash){
	announce_req request;
	request.connection_id = connection_id;
	request.action = htonl(ANNOUNCE);
	request.transaction_id = gen_transaction_id();
	request.info_hash = info_hash;

	request.peer_id;
	request.downloaded;
	request.left;
	request.uploaded;

	request.event = htonl(tracked_directories[infohashes[info_hash]]->announced_times == 0 ? STARTED : NONE);
	request.key = gen_transaction_id();
	request.num_want = htonl(options.get<int32_t>("discovery.bttracker.num_want"));

	request.port;
}

void UDPTrackerConnection::handle_connect(std::shared_ptr<udp_buffer> buffer){
	conn_rep* reply = reinterpret_cast<conn_rep*>(buffer->data());
	connection_id = reply->connection_id;

	BOOST_LOG_TRIVIAL(debug) << "<== UDPTracker (" << tracker_address.host << "): Established connection. Connection ID: " << connection_id;
	reconnect_timer.expires_from_now(std::chrono::seconds(options.get<int>("discovery.bttracker.udp_reconnect_interval")));
	reconnect_timer.async_wait(std::bind(&UDPTrackerConnection::connect, this));
}

void UDPTrackerConnection::handle_announce(std::shared_ptr<udp_buffer> buffer){
	announce_rep* reply = reinterpret_cast<announce_rep*>(buffer.get());
	uint8_t* ext_start = buffer.get()+sizeof(announce_rep);

	std::list<std::pair<address, uint16_t>> discovered_nodes;

	if(reply == htonl(ANNOUNCE)){
		for(auto i = sizeof(announce_rep); i+sizeof(announce_rep_ext4) <= buffer->size(); i+=sizeof(announce_rep_ext4)){
			announce_rep_ext4* rep_ext = reinterpret_cast<announce_rep_ext4*>(buffer.get()+i);
		}
	}else if(reply == htonl(ANNOUNCE6)){

	}
}

void UDPTrackerConnection::handle_error(std::shared_ptr<udp_buffer> buffer){

}

} /* namespace p2p */
} /* namespace librevault */
