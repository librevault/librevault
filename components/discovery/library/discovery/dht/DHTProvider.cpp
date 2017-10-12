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
#include "DHTProvider.h"
#include "../nativeaddr.h"
#include "../rand.h"
#include "DHTWrapper.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Q_LOGGING_CATEGORY(log_dht, "discovery.dht")

namespace librevault {

DHTProvider::DHTProvider(QObject* parent) : GenericProvider(parent) {
  socket4_ = new QUdpSocket(this);
  socket6_ = new QUdpSocket(this);
}

void DHTProvider::start() {
  socket4_->bind(QHostAddress::AnyIPv4, port_);
  socket6_->bind(QHostAddress::AnyIPv6, port_);
  dht_wrapper_ = new DHTWrapper(socket4_, socket6_, getRandomArray(20), this);
  connect(dht_wrapper_, &DHTWrapper::nodeCountChanged, this, &DHTProvider::nodeCountChanged);
  connect(dht_wrapper_, &DHTWrapper::foundNodes, this, &DHTProvider::handleSearch);
  connect(dht_wrapper_, &DHTWrapper::searchDone, this, &DHTProvider::handleSearch);

  dht_wrapper_->enable();
}

void DHTProvider::stop() {
  dht_wrapper_->disable();

  delete dht_wrapper_;
  dht_wrapper_ = nullptr;
  socket4_->close();
  socket6_->close();
}

void DHTProvider::readSession(QIODevice* io) {
  QJsonObject session_json;
  session_json = QJsonDocument::fromJson(io->readAll()).object();
  qCDebug(log_dht) << "DHT session file loaded";

  QJsonArray nodes = session_json["nodes"].toArray();
  qCInfo(log_dht) << "Loading" << nodes.size() << "nodes from session file";
  for (const QJsonValue& node_v : qAsConst(nodes)) {
    QJsonObject node = node_v.toObject();
    addNode(QHostAddress(node["ip"].toString()), node["port"].toInt());
  }
}

void DHTProvider::writeSession(QIODevice* io) {
  QList<Endpoint> nodes = dht_wrapper_->getNodes();
  qCInfo(log_dht) << "Saving" << nodes.count() << "nodes to session file";

  QJsonObject json_object;

  QJsonArray nodes_j;
  for (auto& node : qAsConst(nodes)) {
    QJsonObject node_j;
    node_j["ip"] = node.first.toString();
    node_j["port"] = node.second;
    nodes_j.append(node_j);
  }
  json_object["nodes"] = nodes_j;

  if (io->write(QJsonDocument(json_object).toJson(QJsonDocument::Indented)))
    qCDebug(log_dht) << "DHT session saved";
  else
    qCWarning(log_dht) << "DHT session not saved";
}

int DHTProvider::getNodeCount() const { return dht_wrapper_ ? dht_wrapper_->goodNodeCount() : 0; }

QList<QPair<QHostAddress, quint16>> DHTProvider::getNodes() {
  return (dht_wrapper_) ? dht_wrapper_->getNodes() : QList<QPair<QHostAddress, quint16>>();
}

void DHTProvider::addRouter(QString host, quint16 port) {
  int id = QHostInfo::lookupHost(host, this, SLOT(handleResolve(QHostInfo)));
  resolves_[id] = port;
}

void DHTProvider::addNode(QHostAddress addr, quint16 port) {
  if (dht_wrapper_) dht_wrapper_->pingNode(addr, port);
}

void DHTProvider::startAnnounce(
    QByteArray id, QAbstractSocket::NetworkLayerProtocol af, quint16 port) {
  if (dht_wrapper_) dht_wrapper_->startAnnounce(id, af, port);
}

void DHTProvider::startSearch(QByteArray id, QAbstractSocket::NetworkLayerProtocol af) {
  if (dht_wrapper_) dht_wrapper_->startSearch(id, af);
}

void DHTProvider::handleResolve(const QHostInfo& host) {
  if (host.error()) {
    qCWarning(log_dht) << "Error resolving:" << host.hostName() << "E:" << host.errorString();
    resolves_.remove(host.lookupId());
    return;
  }

  quint16 port = resolves_.take(host.lookupId());

  for (const QHostAddress& address : host.addresses()) {
    addNode(address, port);
    qCDebug(log_dht) << "Added a DHT router:" << host.hostName()
                     << "Resolved:" << address.toString();
  }
}

void DHTProvider::handleSearch(QByteArray id, QAbstractSocket::NetworkLayerProtocol af,
    QList<QPair<QHostAddress, quint16>> nodes) {
  for (auto& endpoint : qAsConst(nodes)) emit discovered(id, endpoint);
}

} /* namespace librevault */
