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
#include "UDPTrackerConnection.h"
#include "BTTrackerDiscovery.h"
#include "nat/PortMappingService.h"
#include "nodekey/NodeKey.h"
#include "util/log.h"
#include <cryptopp/osrng.h>

namespace librevault {

UDPTrackerConnection::UDPTrackerConnection(url tracker_address,
                                           std::shared_ptr<FolderGroup> group_ptr,
                                           BTTrackerDiscovery& tracker_discovery,
                                           NodeKey& node_key, PortMappingService& port_mapping, io_service& io_service) :
	TrackerConnection(tracker_address, group_ptr, tracker_discovery, node_key, port_mapping, io_service),

		socket_(io_service),
		resolver_(io_service),
		reconnect_timer_(io_service),
		announce_timer_(io_service) {
	announce_interval_ = std::chrono::seconds(Config::get()->global_get("bttracker_min_interval").asUInt64());

	if(tracker_address_.port == 0){tracker_address_.port = 80;}

	bind_address_ = address::from_string(url(Config::get()->global_get("p2p_listen").asString()).host);
	socket_.open(bind_address_.is_v6() ? boost::asio::ip::udp::v6() : boost::asio::ip::udp::v4());
	socket_.bind(udp_endpoint(bind_address_, 0));

	LOGD("Resolving IP address");

	udp_resolver::query resolver_query = udp_resolver::query(tracker_address.host, std::to_string(tracker_address.port));
	resolver_.async_resolve(resolver_query, std::bind(&UDPTrackerConnection::handle_resolve, this, std::placeholders::_1, std::placeholders::_2));
}

UDPTrackerConnection::~UDPTrackerConnection() {
	LOGD("UDPTrackerConnection Removed");
}

void UDPTrackerConnection::receive_loop(){
	auto endpoint_ptr = std::make_shared<udp_endpoint>();
	buffer_type buffer_ptr = std::make_shared<std::vector<char>>(buffer_maxsize_);

	socket_.async_receive_from(boost::asio::buffer(buffer_ptr->data(), buffer_ptr->size()), *endpoint_ptr, [this, endpoint_ptr, buffer_ptr](const boost::system::error_code& error, std::size_t bytes_transferred){
		if(error == boost::asio::error::operation_aborted) return;
		try {
			buffer_ptr->resize(bytes_transferred);

			if(buffer_ptr->size() < sizeof(rep_header)) throw invalid_msg_error();
			rep_header* reply_header = reinterpret_cast<rep_header*>(buffer_ptr->data());
			Action reply_action = Action(int32_t(reply_header->action_));

			if(reply_header->transaction_id_ != transaction_id_ || reply_action != action_) throw invalid_msg_error();

			switch(reply_action) {
				case Action::ACTION_CONNECT:
					handle_connect(buffer_ptr);
					break;
				case Action::ACTION_ANNOUNCE:
				case Action::ACTION_ANNOUNCE6:
					handle_announce(buffer_ptr);
					break;
				case Action::ACTION_SCRAPE: /* TODO: Scrape requests */ throw invalid_msg_error();
				case Action::ACTION_ERROR: throw invalid_msg_error();
				default: throw invalid_msg_error();
			}
		}catch(invalid_msg_error& e){
			LOGW("Invalid message received from tracker.");
		}

		receive_loop();
	});
}

void UDPTrackerConnection::bump_reconnect_timer() {
	reconnect_timer_.expires_from_now(std::chrono::seconds(Config::get()->global_get("bttracker_reconnect_interval").asUInt64()));
	reconnect_timer_.async_wait(std::bind(&UDPTrackerConnection::connect, this, std::placeholders::_1));
}

void UDPTrackerConnection::bump_announce_timer() {
	announce_interval_ = std::max(announce_interval_, std::chrono::seconds(Config::get()->global_get("bttracker_min_interval").asUInt64()));

	announce_timer_.expires_from_now(announce_interval_);
	announce_timer_.async_wait(std::bind(&UDPTrackerConnection::announce, this, std::placeholders::_1));
}

void UDPTrackerConnection::connect(const boost::system::error_code& ec) {
	if(ec == boost::asio::error::operation_aborted) return;

	action_ = Action::ACTION_CONNECT;
	transaction_id_ = gen_transaction_id();
	connection_id_ = boost::endian::native_to_big<int64_t>(0x41727101980);

	conn_req request;
	request.header_ = req_header{connection_id_, (int32_t)action_, transaction_id_};

	socket_.async_send_to(boost::asio::buffer((char*)&request, sizeof(request)), target_, std::bind([this](int32_t transaction_id){
		LOGD("Establishing connection. tID=" << transaction_id);
	}, transaction_id_));

	bump_reconnect_timer();
}

void UDPTrackerConnection::announce(const boost::system::error_code& ec) {
	if(ec == boost::asio::error::operation_aborted) return;

	action_ = Action::ACTION_ANNOUNCE;
	transaction_id_ = gen_transaction_id();

	announce_req request;
	request.header_ = req_header{connection_id_, (int32_t)action_, transaction_id_};
	request.info_hash_ = get_info_hash();

	request.peer_id_ = get_peer_id();
	//request.downloaded_;
	//request.left_;
	//request.uploaded_;

	request.event_ = int32_t(announced_times_++ == 0 ? Event::EVENT_STARTED : Event::EVENT_NONE);
	request.key_ = gen_transaction_id();
	request.num_want_ = Config::get()->global_get("bttracker_num_want").asUInt();

	request.port_ = port_mapping_.get_port_mapping("main");

	socket_.async_send_to(boost::asio::buffer((char*)&request, sizeof(request)), target_, std::bind([this](int32_t transaction_id){
		LOGD("Announce sent. tID=" << transaction_id);
	}, transaction_id_));

	bump_announce_timer();
}

void UDPTrackerConnection::handle_resolve(const boost::system::error_code& ec, udp_resolver::iterator iterator){
	if(ec == boost::asio::error::operation_aborted) return;

	for(auto& it = iterator; it != udp_resolver::iterator(); it++)
		if(bind_address_.is_v6() || iterator->endpoint().address().is_v4())
			target_ = iterator->endpoint();
	if(target_ == udp_endpoint()) throw error("Unable to resolve tracker");

	LOGD("Resolved IP: " << target_.address());

	receive_loop();
	connect();
}

void UDPTrackerConnection::handle_connect(buffer_type buffer) {
	bump_reconnect_timer();

	if(buffer->size() == sizeof(conn_rep)) {
		conn_rep* reply = reinterpret_cast<conn_rep*>(buffer->data());
		connection_id_ = reply->connection_id_;
		action_ = Action::ACTION_NONE;
		fail_count_ = 0;

		LOGD("Connection established. cID=" << connection_id_);

		announce();
	}else
		throw invalid_msg_error();
}

void UDPTrackerConnection::handle_announce(buffer_type buffer) {
	if(buffer->size() >= sizeof(announce_rep)) {
		announce_rep* reply = reinterpret_cast<announce_rep*>(buffer->data());
		const char* endpoint_array_ptr = reinterpret_cast<const char*>(buffer->data() + sizeof(announce_rep));
		const size_t endpoint_array_bytesize = buffer->size() - sizeof(announce_rep);

		if(reply->header_.action_ == (int32_t)Action::ACTION_ANNOUNCE) {
			for(size_t i = 0; i < (endpoint_array_bytesize / sizeof(btcompat::compact_endpoint4)); i++)
				tracker_discovery_.add_node(btcompat::parse_compact_endpoint4(reinterpret_cast<const uint8_t*>(endpoint_array_ptr+(i*sizeof(btcompat::compact_endpoint4)))), group_ptr_);
		}else if(reply->header_.action_ == (int32_t)Action::ACTION_ANNOUNCE6) {
			// TODO: Implement IPv6 tracker discovery
		}

		announce_interval_ = std::chrono::seconds(reply->interval_);
		bump_announce_timer();
	}else
		throw invalid_msg_error();
}

int32_t UDPTrackerConnection::gen_transaction_id() {
	CryptoPP::AutoSeededRandomPool rng;
	int32_t number;
	rng.GenerateBlock(reinterpret_cast<uint8_t*>(&number), sizeof(number));
	return number;
}

} /* namespace librevault */
