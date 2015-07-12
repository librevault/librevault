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
#include "../../Session.h"

#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>
#include <boost/lexical_cast.hpp>
#include <cryptopp/osrng.h>
#include <cmath>

namespace librevault {
namespace p2p {

UDPTrackerConnection::UDPTrackerConnection(url tracker_address, Session& session) :
		TrackerConnection(std::move(tracker_address), session),
		socket(session.get_ios()),
		resolver(session.get_ios()), timer(session.get_ios()) {
	assert(this->tracker_address.scheme == "udp");
	if(this->tracker_address.port == 0){this->tracker_address.port = 80;}

	log = spdlog::stderr_logger_mt(std::string("discovery.bttracker.udp(") + (std::string)this->tracker_address + ")");

	bind_address = address::from_string(session.get_options().get<std::string>("net.ip"));
	socket.open(bind_address.is_v6() ? udp::v6() : udp::v4());
	socket.bind(udp::endpoint(bind_address, 0));

	log->debug() << "==> Resolving IP address";

	udp::resolver::query resolver_query = udp::resolver::query(tracker_address.host, boost::lexical_cast<std::string>(tracker_address.port));
	resolver.async_resolve(resolver_query, std::bind(&UDPTrackerConnection::handle_resolve, this, std::placeholders::_1, std::placeholders::_2));

	receive_loop();

	log->debug() << "Added";
}

UDPTrackerConnection::~UDPTrackerConnection() {
	log->debug() << "Removed";
}

void UDPTrackerConnection::handle_resolve(const boost::system::error_code& error, udp::resolver::iterator iterator){
	for(auto& it = iterator; it != udp::resolver::iterator(); it++)
		if(bind_address.is_v6() || iterator->endpoint().address().is_v4())
			target = iterator->endpoint();
	if(target == udp::endpoint()) throw std::runtime_error("Unable to resolve tracker");

	log->debug() << "<== IP: " << target.address();
	connect();
}

void UDPTrackerConnection::receive_loop(){
	auto buffer_ptr = std::make_shared<udp_buffer>(udp_buffer_max_size);
	auto endpoint_ptr = std::make_shared<udp::endpoint>();
	socket.async_receive_from(boost::asio::buffer((char*)buffer_ptr->data(), buffer_ptr->size()), *endpoint_ptr, std::bind([this](const boost::system::error_code& error, std::size_t bytes_transferred, std::shared_ptr<udp_buffer> buffer_ptr, std::shared_ptr<udp::endpoint> endpoint_ptr){
		if(error == boost::asio::error::operation_aborted) return;
		buffer_ptr->resize(bytes_transferred);

		int32_t transaction_rcvd = *reinterpret_cast<int32_t*>(buffer_ptr->data()+4);
		if(transaction_rcvd != transaction_id) receive_loop(); return;

		Action action_rcvd = (Action)ntohl(*reinterpret_cast<int32_t*>(buffer_ptr->data()));
		if(action_rcvd != action || action_rcvd == Action::ERROR) receive_loop(); return;

		switch(action_rcvd){
		case Action::CONNECT:
			handle_connect(buffer_ptr); break;
		case Action::ANNOUNCE:
		case Action::ANNOUNCE6:
			handle_announce(buffer_ptr); break;
		case Action::SCRAPE: /* TODO: Scrape requests */ break;
		case Action::ERROR:
			handle_error(buffer_ptr); break;
		}

		receive_loop();
	}, std::placeholders::_1, std::placeholders::_2, buffer_ptr, endpoint_ptr));
}

void UDPTrackerConnection::start_timer(){
	if(action != Action::NONE){
		std::chrono::seconds timeout = std::chrono::seconds(session.get_options().get<unsigned int>("discovery.bttracker.udp.packet_timeout"));
		timeout *= std::pow(2, fail_count > 8 ? 8 : fail_count);

		timer.cancel();
		timer.expires_from_now(timeout);
		timer.async_wait([this](const boost::system::error_code& error){
			if(error == boost::asio::error::operation_aborted) return;
			fail_count++;
			connect();
		});
	}else{
		time_point reconnect_deadline = connected_time+std::chrono::seconds(session.get_options().get<unsigned int>("discovery.bttracker.udp.reconnect_interval"));
		timer.cancel();

		if(get_queued_time() < reconnect_deadline){
			timer.expires_at(get_queued_time());
			timer.async_wait(std::bind(&UDPTrackerConnection::announce, this, std::placeholders::_1));
		}else{
			timer.expires_at(reconnect_deadline);
			timer.async_wait(std::bind(&UDPTrackerConnection::connect, this, std::placeholders::_1));
		}
	}
}

void UDPTrackerConnection::connect(const boost::system::error_code& error){
	if(error == boost::asio::error::operation_aborted) return;

	action = Action::CONNECT;
	transaction_id = gen_transaction_id();
	connection_id = boost::endian::native_to_big<int64_t>(0x41727101980);

	conn_req request;
	request.header = req_header{connection_id, (int32_t)action, transaction_id};

	socket.async_send_to(boost::asio::buffer((char*)&request, sizeof(request)), target, std::bind([this](int32_t transaction_id){
		log->debug() << "Establishing connection. tID=" << transaction_id;
	}, transaction_id));
}

void UDPTrackerConnection::handle_connect(std::shared_ptr<udp_buffer> buffer){
	conn_rep* reply = reinterpret_cast<conn_rep*>(buffer->data());
	connection_id = reply->connection_id;
	action = Action::NONE;
	fail_count = 0;

	connected_time = std::chrono::steady_clock::now();

	log->debug() << "Connection established. cID=" << connection_id;
//	reconnect_timer.expires_from_now(std::chrono::seconds(options.get<int>("discovery.bttracker.udp_reconnect_interval")));
//	reconnect_timer.async_wait(std::bind(&UDPTrackerConnection::connect, this));
}

void UDPTrackerConnection::announce(const boost::system::error_code& error){
	if(error == boost::asio::error::operation_aborted) return;
	if(announce_queue.top()->dir_ptr == nullptr){ start_timer(); return; }

	action = Action::ANNOUNCE;
	transaction_id = gen_transaction_id();

	announce_req request;
	request.header = req_header{connection_id, (int32_t)action, transaction_id};
	request.info_hash = get_infohash(announce_queue.top()->dir_ptr->get_key());

	request.peer_id;
	request.downloaded;
	request.left;
	request.uploaded;

	request.event = int32_t(tracked_directories[infohashes[request.info_hash]]->announced_times == 0 ? Event::STARTED : Event::NONE);
	request.key = gen_transaction_id();
	request.num_want = session.get_options().get<int32_t>("discovery.bttracker.num_want");

	request.port;
}

void UDPTrackerConnection::handle_announce(std::shared_ptr<udp_buffer> buffer){
	announce_rep* reply = reinterpret_cast<announce_rep*>(buffer.get());
	uint8_t* ext_start = buffer->data()+sizeof(announce_rep);

	std::list<std::pair<address, uint16_t>> discovered_nodes;

	if(reply->header.action == (int32_t)Action::ANNOUNCE){
		for(auto i = sizeof(announce_rep); i+sizeof(announce_rep_ext4) <= buffer->size(); i+=sizeof(announce_rep_ext4)){
			announce_rep_ext4* rep_ext = reinterpret_cast<announce_rep_ext4*>(buffer.get()+i);
		}
	}else if(reply->header.action == (int32_t)Action::ANNOUNCE6){

	}
}

void UDPTrackerConnection::handle_error(std::shared_ptr<udp_buffer> buffer){

}

int32_t UDPTrackerConnection::gen_transaction_id() {
	CryptoPP::AutoSeededRandomPool rng;
	int32_t number;
	rng.GenerateBlock(reinterpret_cast<uint8_t*>(&number), sizeof(number));
	return number;
}

} /* namespace p2p */
} /* namespace librevault */
