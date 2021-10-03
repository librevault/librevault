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
 */
#include "MLDHTProvider.h"

#include <cryptopp/osrng.h>
#include <dht.h>

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QtNetwork/QNetworkDatagram>
#include <boost/asio/ip/address.hpp>

#include "control/Config.h"
#include "control/Paths.h"
#include "control/StateCollector.h"
#include "nat/PortMappingService.h"
#include "util/parse_url.h"

Q_LOGGING_CATEGORY(log_dht, "discovery.dht")

namespace librevault {

MLDHTProvider::MLDHTProvider(PortMappingService* port_mapping, QObject* parent)
    : QObject(parent), port_mapping_(port_mapping) {
  qRegisterMetaType<btcompat::info_hash>("btcompat::info_hash");

  socket_ = new QUdpSocket(this);
  periodic_ = new QTimer(this);
  periodic_->setSingleShot(true);
  periodic_->setTimerType(Qt::VeryCoarseTimer);

  connect(socket_, &QUdpSocket::readyRead, this, &MLDHTProvider::processDatagram);
  connect(periodic_, &QTimer::timeout, this, &MLDHTProvider::periodicRequest);

  init();
}

MLDHTProvider::~MLDHTProvider() { deinit(); }

void MLDHTProvider::init() {
  // We will restore our session from here
  readSessionFile();

  // Init sockets
  socket_->bind(getPort());

  int rc = dht_init(socket_->socketDescriptor(), socket_->socketDescriptor(), own_id.data(), nullptr);
  if (rc < 0) qCWarning(log_dht) << "Could not initialize DHT: Internal DHT error";

  // Map port
  port_mapping_->map({"mldht", getPort(), QAbstractSocket::UdpSocket, "Librevault DHT"});

  // Init routers
  for (const QString& router_value : Config::get()->getGlobal("mainline_dht_routers").toStringList()) {
    url router_url(router_value);
    QHostInfo::lookupHost(router_url.host, this, [=, this](const QHostInfo& host) {
      if (host.error()) qCWarning(log_dht) << "Error resolving:" << host.hostName() << "E:" << host.errorString();

      for (const auto& address : host.addresses()) {
        Endpoint endpoint(address, router_url.port);
        addNode(endpoint);
        qCDebug(log_dht) << "Added a DHT router:" << host.hostName() << "Resolved:" << endpoint;
      }
    });
  }

  periodic_->start();
}

void MLDHTProvider::deinit() {
  periodic_->stop();

  writeSessionFile();
  dht_uninit();

  socket_->close();

  port_mapping_->unmap("mldht");
}

void MLDHTProvider::readSessionFile() {
  QJsonObject session_json;
  QFile session_f(Paths::get()->dht_session_path);
  if (session_f.open(QIODevice::ReadOnly)) {
    session_json = QJsonDocument::fromJson(session_f.readAll()).object();
    qCDebug(log_dht) << "DHT session file loaded";
  }

  // Init id
  QByteArray own_id_arr =
      session_json["id"].isString() ? QByteArray::fromBase64(session_json["id"].toString().toLatin1()) : QByteArray();
  if (own_id_arr.size() == (int)own_id.size())
    std::copy(own_id_arr.begin(), own_id_arr.end(), own_id.begin());
  else  // Invalid data
    CryptoPP::AutoSeededRandomPool().GenerateBlock(own_id.data(), own_id.size());

  QJsonArray nodes = session_json["nodes"].toArray();
  qCInfo(log_dht) << "Loading" << nodes.size() << "nodes from session file";
  for (const QJsonValue& node : nodes) addNode(Endpoint::fromJson(node.toObject()));
}

void MLDHTProvider::writeSessionFile() {
  QByteArray own_id_arr((const char*)own_id.data(), own_id.size());

  struct sockaddr_in6 sa6[300];
  struct sockaddr_in sa4[300];

  int sa6_count = 300;
  int sa4_count = 300;

  dht_get_nodes(sa4, &sa4_count, sa6, &sa6_count);

  qCInfo(log_dht) << "Saving" << sa4_count + sa6_count << "nodes to session file";

  QJsonObject json_object;

  QJsonArray nodes;
  for (auto i = 0; i < sa6_count; i++) nodes += Endpoint::fromSockaddr((sockaddr&)sa6[i]).toJson();
  for (auto i = 0; i < sa4_count; i++) nodes += Endpoint::fromSockaddr((sockaddr&)sa4[i]).toJson();

  json_object["nodes"] = nodes;
  json_object["id"] = QString::fromLatin1(own_id_arr.toBase64());

  QFile session_f(Paths::get()->dht_session_path);
  if (session_f.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
      session_f.write(QJsonDocument(json_object).toJson(QJsonDocument::Indented)))
    qCDebug(log_dht) << "DHT session saved";
  else
    qCWarning(log_dht) << "DHT session not saved";
}

int MLDHTProvider::nodeCount() const {
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

  return good6 + good4 + dubious6 + dubious4;
}

quint16 MLDHTProvider::getPort() { return (quint16)Config::get()->getGlobal("mainline_dht_port").toUInt(); }

quint16 MLDHTProvider::getExternalPort() { return port_mapping_->mappedPortOrOriginal("main"); }

void MLDHTProvider::addNode(const Endpoint& endpoint) {
  if (endpoint.addr.isNull()) return;
  auto [sa, size] = endpoint.toSockaddr();
  dht_ping_node((const sockaddr*)&sa, size);
}

void MLDHTProvider::passCallback(void* closure, int event, const uint8_t* info_hash, const QByteArray& data) {
  qCDebug(log_dht) << BOOST_CURRENT_FUNCTION << "event:" << event;

  btcompat::info_hash ih;
  std::copy(info_hash, info_hash + ih.size(), ih.begin());

  if (event == DHT_EVENT_NONE) return;
  emit eventReceived(event, ih, data);
}

void MLDHTProvider::processDatagram() {
  auto datagram = socket_->receiveDatagram();
  auto data = datagram.data();
  auto [sa, size] = Endpoint(datagram.senderAddress(), datagram.senderPort()).toSockaddr();

  time_t tosleep;
  dht_periodic(data, data.size(), (const sockaddr*)&sa, (int)size, &tosleep, lv_dht_callback_glue, this);
  emit nodeCountChanged(nodeCount());

  periodic_->start(tosleep * 1000);
}

void MLDHTProvider::periodicRequest() {
  time_t tosleep;
  dht_periodic(nullptr, 0, nullptr, 0, &tosleep, lv_dht_callback_glue, this);
  emit nodeCountChanged(nodeCount());

  periodic_->start(tosleep * 1000);
}

}  // namespace librevault

// DHT library overrides
extern "C" {

int dht_blacklisted(const struct sockaddr* sa, int salen) {
  //	for(int i = 0; i < salen; i++) {
  //		QHostAddress peer_addr(sa+i);
  //	}
  return 0;
}

void dht_hash(void* hash_return, int hash_size, const void* v1, int len1, const void* v2, int len2, const void* v3,
              int len3) {
  std::fill((char*)hash_return, (char*)hash_return + hash_size, 0);

  QCryptographicHash hasher(QCryptographicHash::Sha1);
  hasher.addData((const char*)v1, len1);
  hasher.addData((const char*)v2, len2);
  hasher.addData((const char*)v3, len3);
  QByteArray result = hasher.result();

  std::copy(result.begin(), result.begin() + qMin(result.size(), hash_size), (char*)hash_return);
}

int dht_random_bytes(void* buf, size_t size) {
  CryptoPP::AutoSeededRandomPool().GenerateBlock((uint8_t*)buf, size);
  return size;
}

void lv_dht_callback_glue(void* closure, int event, const unsigned char* info_hash, const void* data, size_t data_len) {
  ((librevault::MLDHTProvider*)closure)
      ->passCallback(closure, event, info_hash, QByteArray((const char*)data, data_len));
}

} /* extern "C" */
