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
#include "MLDHTProvider.h"
#include "discovery/mldht/dht_glue.h"
#include "control/Paths.h"
#include "control/StateCollector.h"
#include "nat/PortMappingService.h"
#include "util/log.h"
#include "util/file_util.h"
#include "util/parse_url.h"
#include <MLDHTSessionFile.pb.h>
#include <dht.h>
#include <cryptopp/osrng.h>

namespace librevault {

using namespace boost::asio::ip;

MLDHTProvider::MLDHTProvider(PortMappingService* port_mapping, StateCollector* state_collector, QObject* parent) : QObject(parent),
	port_mapping_(port_mapping),
	state_collector_(state_collector) {

	qRegisterMetaType<btcompat::info_hash>("btcompat::info_hash");

	socket_ = new QUdpSocket(this);
	periodic_ = new QTimer(this);
	periodic_->setSingleShot(true);

	connect(socket_, &QUdpSocket::readyRead, this, &MLDHTProvider::processDatagram);
	connect(periodic_, &QTimer::timeout, this, &MLDHTProvider::periodic_request);

	init();
}

MLDHTProvider::~MLDHTProvider() {
	deinit();
}

void MLDHTProvider::init() {
	// We will restore our session from here
	readSessionFile();

	// Init sockets
	socket_->bind(getPort());

	int rc = dht_init(socket_->socketDescriptor(), socket_->socketDescriptor(), own_id.data(), nullptr);
	if(rc < 0)
		LOGW("Could not initialize DHT: Internal DHT error");

	// Map port
	port_mapping_->add_port_mapping("mldht", {getPort(), QAbstractSocket::UdpSocket}, "Librevault DHT");

	// Init routers
	auto routers = Config::get()->global_get("mainline_dht_routers");
	if(routers.isArray()) {
		for(auto& router_value : routers) {
			url router_url(router_value.asString());
			int id = QHostInfo::lookupHost(QString::fromStdString(router_url.host), this, SLOT(handle_resolve(QHostInfo)));
			resolves_[id] = router_url.port;
		}
	}

	periodic_->start();
}

void MLDHTProvider::deinit() {
	periodic_->stop();

	writeSessionFile();
	dht_uninit();

	socket_->close();

	port_mapping_->remove_port_mapping("mldht");
}

void MLDHTProvider::readSessionFile() {
	MLDHTSessionFile session_pb;
	file_wrapper session_file(Paths::get()->dht_session_path, "r");
	if(!session_pb.ParseFromIstream(&session_file.ios())) throw;

	// Init id
	if(session_pb.id().size() == own_id.size()) {
		std::copy(session_pb.id().begin(), session_pb.id().end(), own_id.begin());
	}else{  // Invalid data
		CryptoPP::AutoSeededRandomPool().GenerateBlock(own_id.data(), own_id.size());
	}

	std::vector<tcp_endpoint> nodes;
	nodes.reserve(session_pb.compact_endpoints6().size()+session_pb.compact_endpoints4().size());

	LOGI("Loading " << session_pb.compact_endpoints6().size()+session_pb.compact_endpoints4().size() << " nodes from session file");
	for(auto& compact_node4 : session_pb.compact_endpoints4()) {
		if(compact_node4.size() != sizeof(btcompat::compact_endpoint4)) continue;
		nodes.push_back(btcompat::parse_compact_endpoint6(compact_node4.data()));
	}
	for(auto& compact_node6 : session_pb.compact_endpoints6()) {
		if(compact_node6.size() != sizeof(btcompat::compact_endpoint6)) continue;
		nodes.push_back(btcompat::parse_compact_endpoint6(compact_node6.data()));
	}

	for(auto& endpoint : nodes)
		addNode(endpoint);
}

void MLDHTProvider::writeSessionFile() {
	MLDHTSessionFile session_pb;
	std::copy(own_id.begin(), own_id.end(), std::back_inserter(*session_pb.mutable_id()));

	struct sockaddr_in6 sa6[300];
	struct sockaddr_in sa4[300];

	int sa6_count = 300;
	int sa4_count = 300;

	dht_get_nodes(sa4, &sa4_count, sa6, &sa6_count);

	LOGI("Saving " << sa4_count + sa6_count << " nodes to session file");

	for(auto i=0; i < sa6_count; i++) {
		btcompat::compact_endpoint6 endpoint6;
		std::copy((uint8_t*)&sa6[i].sin6_addr, (uint8_t*)&(sa6[i].sin6_addr)+16, endpoint6.ip6.data());
		endpoint6.port = boost::endian::big_to_native(sa6[i].sin6_port);
		std::string* endpoint_str = session_pb.add_compact_endpoints6();
		std::copy((const char*)&endpoint6, ((const char*)&endpoint6)+sizeof(btcompat::compact_endpoint6), std::back_inserter(*endpoint_str));
	}
	for(auto i=0; i < sa4_count; i++) {
		btcompat::compact_endpoint4 endpoint4;
		std::copy((uint8_t*)&sa4[i].sin_addr, (uint8_t*)&(sa4[i].sin_addr)+4, endpoint4.ip4.data());
		endpoint4.port = boost::endian::big_to_native(sa4[i].sin_port);
		std::string* endpoint_str = session_pb.add_compact_endpoints4();
		std::copy((const char*)&endpoint4, ((const char*)&endpoint4)+sizeof(btcompat::compact_endpoint4), std::back_inserter(*endpoint_str));
	}

	file_wrapper session_file(Paths::get()->dht_session_path, "w");
	session_pb.SerializeToOstream(&session_file.ios());
}

uint_least32_t MLDHTProvider::node_count() const {
	int good6 = 0;
	int dubious6 = 0;
	int cached6 = 0;
	int incoming6 = 0;
	int good4 = 0;
	int dubious4 = 0;
	int cached4 = 0;
	int incoming4 = 0;

	dht_nodes(AF_INET6, &good6, &dubious6, &cached6, &incoming6);
	dht_nodes(AF_INET, &good4, &dubious4, &cached4, &incoming4);

	return good6+good4;
}

quint16 MLDHTProvider::getPort() {
	return (quint16)Config::get()->global_get("mainline_dht_port").asUInt();
}

quint16 MLDHTProvider::getExternalPort() {
	return port_mapping_->get_port_mapping("main");
}

void MLDHTProvider::addNode(tcp_endpoint endpoint) {
	dht_ping_node(endpoint.data(), endpoint.size());
}

void MLDHTProvider::pass_callback(void* closure, int event, const uint8_t* info_hash, const uint8_t* data, size_t data_len) {
	LOGD(BOOST_CURRENT_FUNCTION << " event: " << event);

	btcompat::info_hash ih; std::copy(info_hash, info_hash + ih.size(), ih.begin());

	if(event == DHT_EVENT_VALUES || event == DHT_EVENT_VALUES6)
		emit eventReceived(event, ih, QByteArray((char*)data, data_len));
	else if(event == DHT_EVENT_SEARCH_DONE || event == DHT_EVENT_SEARCH_DONE6)
		emit eventReceived(event, ih, QByteArray());
}

void MLDHTProvider::processDatagram() {
	char datagram_buffer[buffer_size_];
	QHostAddress address;
	quint16 port;
	qint64 datagram_size = socket_->readDatagram(datagram_buffer, buffer_size_, &address, &port);

	tcp_endpoint endpoint(address::from_string(address.toString().toStdString()), port);

	time_t tosleep;
	dht_periodic(datagram_buffer, datagram_size, endpoint.data(), (int)endpoint.size(), &tosleep, lv_dht_callback_glue, this);
	state_collector_->global_state_set("dht_nodes_count", node_count());

	periodic_->setInterval(tosleep*1000);
}

void MLDHTProvider::periodic_request() {
	time_t tosleep;
	dht_periodic(nullptr, 0, nullptr, 0, &tosleep, lv_dht_callback_glue, this);
	state_collector_->global_state_set("dht_nodes_count", node_count());

	periodic_->setInterval(tosleep*1000);
}

void MLDHTProvider::handle_resolve(const QHostInfo& host) {
	if(host.error()) {
		LOGW("Error resolving: " << host.hostName() << " E: " << host.errorString());
		resolves_.remove(host.lookupId());
	}else {
		QHostAddress address = host.addresses().first();
		quint16 port = resolves_.take(host.lookupId());

		addNode(tcp_endpoint(address::from_string(address.toString().toStdString()), port));
		LOGD("Added a DHT router: " << host.hostName() << " Resolved: " << address.toString());
	}
}

} /* namespace librevault */

// DHT library overrides
extern "C" {

int dht_blacklisted(const struct sockaddr *sa, int salen) {
//	for(int i = 0; i < salen; i++) {
//		QHostAddress peer_addr(sa+i);
//	}
	return 0;
}

void dht_hash(void *hash_return, int hash_size, const void *v1, int len1, const void *v2, int len2, const void *v3, int len3) {
	constexpr unsigned sha1_size = 20;

	if(hash_size > (int)sha1_size)
		std::fill((uint8_t*)hash_return, (uint8_t*)hash_return + sha1_size, 0);

	CryptoPP::SHA1 sha1;
	sha1.Update((const uint8_t*)v1, len1);
	sha1.Update((const uint8_t*)v2, len2);
	sha1.Update((const uint8_t*)v3, len3);
	sha1.TruncatedFinal((uint8_t*)hash_return, std::min(sha1.DigestSize(), sha1_size));
}

int dht_random_bytes(void *buf, size_t size) {
	CryptoPP::AutoSeededRandomPool().GenerateBlock((uint8_t*)buf, size);
	return size;
}

} /* extern "C" */
