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
#include "BTTrackerConnection.h"
#include "BTTrackerMessages.h"
#include "BTTrackerProvider.h"
#include "BTTrackerGroup.h"
#include "util/log.h"
#include <cryptopp/osrng.h>

namespace librevault {

BTTrackerConnection::BTTrackerConnection(QUrl tracker_address, BTTrackerGroup* btgroup_, BTTrackerProvider* tracker_provider) : tracker_address_(tracker_address) {
	// Resolve loop
	resolver_timer_ = new QTimer(this);
	resolver_timer_->setInterval(30*1000);
	connect(resolver_timer_, &QTimer::timeout, this, &BTTrackerConnection::resolve);

	// Connect loop
	connect_timer_ = new QTimer(this);
	connect_timer_->setInterval(Config::get()->global_get("bttracker_min_interval").asUInt64()*1000);
	connect(connect_timer_, &QTimer::timeout, this, &BTTrackerConnection::btconnect);

	// Announcer loop
	announce_timer_ = new QTimer(this);
	announce_timer_->setInterval(30*1000);
	connect(announce_timer_, &QTimer::timeout, this, &BTTrackerConnection::announce);

	connect(tracker_provider, &BTTrackerProvider::receivedMessage, this, &BTTrackerConnection::handle_message);
	connect(this, &BTTrackerConnection::discovered, btgroup_, &BTTrackerGroup::discovered);
}

BTTrackerConnection::~BTTrackerConnection() {
	LOGD("BTTrackerConnection Removed");
}

void BTTrackerConnection::setEnabled(bool enabled) {
	if(enabled) {
		QTimer::singleShot(0, this, &BTTrackerConnection::resolve);
		resolver_timer_->start();
	}else{
		announce_timer_->stop();
		connect_timer_->stop();
		resolver_timer_->stop();
	}
}

void BTTrackerConnection::resolve() {
	if(resolver_lookup_id_) {
		QHostInfo::abortHostLookup(resolver_lookup_id_);
		LOGD("Could not resolve IP address for: " << tracker_address_.host().toStdString());
	}

	LOGD("Resolving IP address for: " << tracker_address_.host().toStdString());
	resolver_lookup_id_ = QHostInfo::lookupHost(tracker_address_.host(), this, SLOT(handle_resolve(QHostInfo)));
}

void BTTrackerConnection::btconnect() {
	// Set internal state
	transaction_id_connect_ = gen_transaction_id();
	connection_id_ = qToBigEndian(0x41727101980ULL);

	// Generate request
	bttracker::conn_req request;
	request.connection_id_ = connection_id_;
	request.transaction_id_ = transaction_id_connect_;
	request.action_ = (quint32)bttracker::Action::ACTION_CONNECT;
	QByteArray message(reinterpret_cast<char*>(&request), sizeof(request));

	socket_->writeDatagram(message, addr_, port_);
}

void BTTrackerConnection::announce() {
	/* Announce via IPv4 */
	// Set internal state
	transaction_id_announce4_ = gen_transaction_id();

	// Generate request
	bttracker::announce_req request4;
	request4.connection_id_ = connection_id_;
	request4.transaction_id_ = transaction_id_announce4_;
	request4.action_ = (quint32)bttracker::Action::ACTION_ANNOUNCE;
	request4.info_hash_ = btgroup_->getInfoHash();
	request4.peer_id_ = provider_->getPeerId();
	//request.downloaded_;
	//request.left_;
	//request.uploaded_;
	request4.event_ = quint32(announced_times_++ == 0 ? bttracker::Event::EVENT_STARTED : bttracker::Event::EVENT_NONE);
	request4.key_ = gen_transaction_id();
	request4.num_want_ = Config::get()->global_get("bttracker_num_want").asUInt();
	request4.port_ = provider_->getExternalPort();
	QByteArray message4(reinterpret_cast<char*>(&request4), sizeof(request4));

	socket_->writeDatagram(message4, addr_, port_);

	/* Announce via IPv6 */
	// Set internal state
	transaction_id_announce6_ = gen_transaction_id();

	// Generate request
	bttracker::announce_req6 request6;
	request6.connection_id_ = connection_id_;
	request6.transaction_id_ = transaction_id_announce6_;
	request6.action_ = (quint32)bttracker::Action::ACTION_ANNOUNCE;
	request6.info_hash_ = btgroup_->getInfoHash();
	request6.peer_id_ = provider_->getPeerId();
	//request.downloaded_;
	//request.left_;
	//request.uploaded_;
	request6.event_ = quint32(announced_times_++ == 0 ? bttracker::Event::EVENT_STARTED : bttracker::Event::EVENT_NONE);
	request6.key_ = gen_transaction_id();
	request6.num_want_ = Config::get()->global_get("bttracker_num_want").asUInt();
	request6.port_ = provider_->getExternalPort();
	QByteArray message6(reinterpret_cast<char*>(&request6), sizeof(request6));

	socket_->writeDatagram(message6, addr_, port_);
}

quint32 BTTrackerConnection::gen_transaction_id() {
	CryptoPP::AutoSeededRandomPool rng;
	quint32 number;
	rng.GenerateBlock(reinterpret_cast<uint8_t*>(&number), sizeof(number));
	return number;
}

void BTTrackerConnection::handle_message(quint32 action, quint32 transaction_id, QByteArray message) {
	bttracker::Action action_bt = (bttracker::Action)action;
	if(action_bt == bttracker::Action::ACTION_CONNECT && transaction_id == transaction_id_connect_)
		handle_connect(message);
	else if(action_bt == bttracker::Action::ACTION_ANNOUNCE && transaction_id == transaction_id_announce4_)
		handle_announce(message);
	else if(action_bt == bttracker::Action::ACTION_ANNOUNCE6 && transaction_id == transaction_id_announce6_)
		handle_announce(message);
}

void BTTrackerConnection::handle_resolve(const QHostInfo& host) {
	resolver_lookup_id_ = 0;
	if(host.error()) {
		LOGD("Could not resolve IP address for: " << tracker_address_.host().toStdString() << " E:" << host.errorString().toStdString());
		resolve();
	}else{
		addr_ = host.addresses().first();
		port_ = tracker_address_.port(80);

		resolver_timer_->stop();
		QTimer::singleShot(0, this, &BTTrackerConnection::btconnect);
		connect_timer_->start();
	}
}

void BTTrackerConnection::handle_connect(QByteArray message) {
	if(message.size() == sizeof(bttracker::conn_rep)) {
		bttracker::conn_rep* message_s((bttracker::conn_rep*)message.data());
		connection_id_ = message_s->connection_id_;
		announce_timer_->start();
	}
}

void BTTrackerConnection::handle_announce(QByteArray message) {
	if((size_t)message.size() >= sizeof(bttracker::announce_rep)) {
		bttracker::announce_rep* message_s((bttracker::announce_rep*)message.data());
		bttracker::Action action_bt = bttracker::Action((uint32_t)message_s->action_);

		QList<std::pair<QHostAddress, quint16>> endpoint_list;
		switch(action_bt) {
			case bttracker::Action::ACTION_ANNOUNCE:
				endpoint_list = btcompat::parseCompactEndpoint4List(message.mid(sizeof(bttracker::announce_rep)));
				break;
			case bttracker::Action::ACTION_ANNOUNCE6:
				endpoint_list = btcompat::parseCompactEndpoint6List(message.mid(sizeof(bttracker::announce_rep)));
				break;
			default:;
		}

		foreach(auto& endpoint, endpoint_list) {
			DiscoveryResult result;
			result.address = endpoint.first;
			result.port = endpoint.second;
			emit discovered(result);
		}

		announce_timer_->setInterval(std::max(Config::get()->global_get("bttracker_min_interval").asUInt()*1000, (quint32)message_s->interval_)*1000);
	}
}

} /* namespace librevault */
