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
#include "DHTWrapper.h"
#include "Discovery.h"
#include "rand.h"
#include "nativeaddr.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

Q_LOGGING_CATEGORY(log_dht, "discovery.dht")

namespace librevault {

MLDHTProvider::MLDHTProvider(Discovery* parent) :
	QObject(parent),
	parent_(parent) {

	socket4_ = new QUdpSocket(this);
	socket6_ = new QUdpSocket(this);
}

MLDHTProvider::~MLDHTProvider() {
	stop();
}

void MLDHTProvider::start(quint16 port) {
	stop(); // Cleanup if initialized

	socket4_->bind(QHostAddress::AnyIPv4, port);
	socket6_->bind(QHostAddress::AnyIPv6, port);
	dht_wrapper_ = new DHTWrapper(socket4_, socket6_, getRandomArray(20), this);
	connect(dht_wrapper_, &DHTWrapper::nodeCountChanged, this, &MLDHTProvider::nodeCountChanged);
	connect(dht_wrapper_, &DHTWrapper::foundNodes, this, &MLDHTProvider::handleSearch);
	connect(dht_wrapper_, &DHTWrapper::searchDone, this, &MLDHTProvider::handleSearch);
	dht_wrapper_->enable();
}

void MLDHTProvider::stop() {
	if(dht_wrapper_) dht_wrapper_->disable();

	delete dht_wrapper_;
	dht_wrapper_ = nullptr;
	socket4_->close();
	socket6_->close();
}

void MLDHTProvider::readSessionFile(QString path) {
	QJsonObject session_json;
	QFile session_f(path);
	if(session_f.open(QIODevice::ReadOnly)) {
		session_json = QJsonDocument::fromJson(session_f.readAll()).object();
		qCDebug(log_dht) << "DHT session file loaded";
	}

	QJsonArray nodes = session_json["nodes"].toArray();
	qCInfo(log_dht) << "Loading" << nodes.size() << "nodes from session file";
	foreach(const QJsonValue& node_v, nodes) {
		QJsonObject node = node_v.toObject();
		addNode(QHostAddress(node["ip"].toString()), node["port"].toInt());
	}
}

void MLDHTProvider::writeSessionFile(QString path) {
	QList<QPair<QHostAddress, quint16>> nodes = dht_wrapper_->getNodes();
	qCInfo(log_dht) << "Saving" << nodes.count() << "nodes to session file";

	QJsonObject json_object;

	QJsonArray nodes_j;
	for(auto& node : nodes) {
		QJsonObject node_j;
		node_j["ip"] = node.first.toString();
		node_j["port"] = node.second;
		nodes_j.append(node_j);
	}
	json_object["nodes"] = nodes_j;

	QFile session_f(path);
	if(session_f.open(QIODevice::WriteOnly | QIODevice::Truncate) && session_f.write(QJsonDocument(json_object).toJson(QJsonDocument::Indented)))
		qCDebug(log_dht) << "DHT session saved";
	else
		qCWarning(log_dht) << "DHT session not saved";
}

int MLDHTProvider::getNodeCount() const {
	return dht_wrapper_->goodNodeCount();
}

QList<QPair<QHostAddress, quint16>> MLDHTProvider::getNodes() {
	return dht_wrapper_->getNodes();
}

void MLDHTProvider::addRouter(QString host, quint16 port) {
	int id = QHostInfo::lookupHost(host, this, SLOT(handleResolve(QHostInfo)));
	resolves_[id] = port;
}

void MLDHTProvider::addNode(QHostAddress addr, quint16 port) {
	dht_wrapper_->pingNode(addr, port);
}

void MLDHTProvider::startAnnounce(QByteArray id, QAbstractSocket::NetworkLayerProtocol af, quint16 port) {
	dht_wrapper_->startAnnounce(id, af, port);
}

void MLDHTProvider::startSearch(QByteArray id, QAbstractSocket::NetworkLayerProtocol af) {
	dht_wrapper_->startSearch(id, af);
}

void MLDHTProvider::handleResolve(const QHostInfo& host) {
	if(host.error()) {
		qCWarning(log_dht) << "Error resolving:" << host.hostName() << "E:" << host.errorString();
		resolves_.remove(host.lookupId());
	}else{
		QHostAddress address = host.addresses().first();
		quint16 port = resolves_.take(host.lookupId());

		addNode(address, port);
		qCDebug(log_dht) << "Added a DHT router:" << host.hostName() << "Resolved:" << address.toString();
	}
}

void MLDHTProvider::handleSearch(QByteArray id, QAbstractSocket::NetworkLayerProtocol af, QList<QPair<QHostAddress, quint16>> nodes) {
	for(auto& endpoint : qAsConst(nodes))
		emit discovered(id, endpoint.first, endpoint.second);
}

} /* namespace librevault */
