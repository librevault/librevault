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
#include "UDPTrackerConnection.h"
#include "src/Client.h"
#include "src/folder/p2p/P2PProvider.h"
#include "src/folder/p2p/WSServer.h"
#include "src/folder/fs/FSFolder.h"

#include "src/folder/p2p/discovery/BTTrackerDiscovery.h"

namespace librevault {

UDPTrackerConnection::UDPTrackerConnection(url tracker_address,
                                           std::shared_ptr<FolderGroup> group_ptr,
                                           BTTrackerDiscovery& tracker_discovery,
                                           Client& client) :
	TrackerConnection(tracker_address, group_ptr, tracker_discovery, client),

		socket_(client.network_ios()),
		resolver_(client.network_ios()),
		reconnect_timer_(client.network_ios()),
		announce_timer_(client.network_ios()) {
	announce_interval_ = std::chrono::seconds(Config::get()->client()["bttracker_min_interval"].asUInt64());

	if(tracker_address_.port == 0){tracker_address_.port = 80;}

	bind_address_ = client_.p2p_provider()->ws_server()->local_endpoint().address();
	socket_.open(bind_address_.is_v6() ? boost::asio::ip::udp::v6() : boost::asio::ip::udp::v4());
	socket_.bind(udp_endpoint(bind_address_, 0));

	log_->debug() << log_tag() << "Resolving IP address";

	udp_resolver::query resolver_query = udp_resolver::query(tracker_address.host, boost::lexical_cast<std::string>(tracker_address.port));
	resolver_.async_resolve(resolver_query, std::bind(&UDPTrackerConnection::handle_resolve, this, std::placeholders::_1, std::placeholders::_2));
}

UDPTrackerConnection::~UDPTrackerConnection() {
	log_->debug() << log_tag() << "UDPTrackerConnection Removed";
}

void UDPTrackerConnection::reset_buffer() {
	buffer_.resize(buffer_maxsize_);
}

void UDPTrackerConnection::receive_loop(){
	reset_buffer();

	auto endpoint_ptr = std::make_shared<udp_endpoint>();
	socket_.async_receive_from(boost::asio::buffer(buffer_.data(), buffer_.size()), *endpoint_ptr, std::bind([this](const boost::system::error_code& error, std::size_t bytes_transferred, std::shared_ptr<udp_endpoint> endpoint_ptr){
		if(error == boost::asio::error::operation_aborted) return;
		try {
			buffer_.resize(bytes_transferred);

			if(buffer_.size() < sizeof(rep_header)) throw invalid_msg_error();
			rep_header* reply_header = reinterpret_cast<rep_header*>(buffer_.data());
			Action reply_action = Action(int32_t(reply_header->action_));

			if(reply_header->transaction_id_ != transaction_id_ || reply_action != action_) throw invalid_msg_error();

			switch(reply_action) {
				case Action::ACTION_CONNECT:
					handle_connect();
					break;
				case Action::ACTION_ANNOUNCE:
				case Action::ACTION_ANNOUNCE6:
					handle_announce();
					break;
				case Action::ACTION_SCRAPE: /* TODO: Scrape requests */ throw invalid_msg_error();
				case Action::ACTION_ERROR: throw invalid_msg_error();
				default: throw invalid_msg_error();
			}
		}catch(invalid_msg_error& e){
			log_->warn() << log_tag() << "Invalid message received from tracker.";
		}

		receive_loop();
	}, std::placeholders::_1, std::placeholders::_2, endpoint_ptr));
}

void UDPTrackerConnection::bump_reconnect_timer() {
	reconnect_timer_.expires_from_now(std::chrono::seconds(Config::get()->client()["bttracker_reconnect_interval"].asUInt64()));
	reconnect_timer_.async_wait(std::bind(&UDPTrackerConnection::connect, this, std::placeholders::_1));
}

void UDPTrackerConnection::bump_announce_timer() {
	announce_interval_ = std::max(announce_interval_, std::chrono::seconds(Config::get()->client()["bttracker_min_interval"].asUInt64()));

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
		log_->debug() << log_tag() << "Establishing connection. tID=" << transaction_id;
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
	request.num_want_ = Config::get()->client()["bttracker_num_want"].asUInt();

	request.port_ = client_.p2p_provider()->public_port();

	socket_.async_send_to(boost::asio::buffer((char*)&request, sizeof(request)), target_, std::bind([this](int32_t transaction_id){
		log_->debug() << log_tag() << "Announce sent. tID=" << transaction_id;
	}, transaction_id_));

	bump_announce_timer();
}

void UDPTrackerConnection::handle_resolve(const boost::system::error_code& ec, udp_resolver::iterator iterator){
	if(ec == boost::asio::error::operation_aborted) return;

	for(auto& it = iterator; it != udp_resolver::iterator(); it++)
		if(bind_address_.is_v6() || iterator->endpoint().address().is_v4())
			target_ = iterator->endpoint();
	if(target_ == udp_endpoint()) throw error("Unable to resolve tracker");

	log_->debug() << log_tag() << "Resolved IP: " << target_.address();

	receive_loop();
	connect();
}

void UDPTrackerConnection::handle_connect() {
	bump_reconnect_timer();

	if(buffer_.size() == sizeof(conn_rep)) {
		conn_rep* reply = reinterpret_cast<conn_rep*>(buffer_.data());
		connection_id_ = reply->connection_id_;
		action_ = Action::ACTION_NONE;
		fail_count_ = 0;

		log_->debug() << log_tag() << "Connection established. cID=" << connection_id_;

		announce();
	}else
		throw invalid_msg_error();
}

void UDPTrackerConnection::handle_announce() {
	announce_rep* reply = reinterpret_cast<announce_rep*>(buffer_.data());

	if(reply->header_.action_ == (int32_t)Action::ACTION_ANNOUNCE){
		for(char* i = buffer_.data()+sizeof(announce_rep); i+sizeof(announce_rep_ext4) <= buffer_.data()+buffer_.size(); i+=sizeof(announce_rep_ext4)){
			announce_rep_ext4* rep_ext = reinterpret_cast<announce_rep_ext4*>(i);

			boost::asio::ip::address_v4 address(rep_ext->ip4_);
			tcp_endpoint endpoint(address, rep_ext->port_);

			log_->debug() << log_tag() << "Discovered node: " << endpoint;

			tracker_discovery_.add_node(endpoint, group_ptr_);
		}
	}else if(reply->header_.action_ == (int32_t)Action::ACTION_ANNOUNCE6){
		// TODO: Implement IPv6 tracker discovery
	}

	announce_interval_ = std::chrono::seconds(reply->interval_);
	bump_announce_timer();
}

int32_t UDPTrackerConnection::gen_transaction_id() {
	CryptoPP::AutoSeededRandomPool rng;
	int32_t number;
	rng.GenerateBlock(reinterpret_cast<uint8_t*>(&number), sizeof(number));
	return number;
}

} /* namespace librevault */
