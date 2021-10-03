/* Copyright (C) 2017 Alexander Shishenko <alex@shishenko.com>
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
#include "Endpoint.h"

#include <QRegularExpression>
#include <QtEndian>
#ifdef Q_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/ip.h>
#include <sys/socket.h>
#endif

namespace librevault {

namespace {
QRegularExpression endpoint_regexp(R"(^\[?([[:xdigit:].:]{2,45})\]?:(\d{1,5})$)");
}

Endpoint::Endpoint(const QHostAddress& addr, quint16 port) : addr(addr), port(port) {}
Endpoint::Endpoint(const QString& addr, quint16 port) : Endpoint(QHostAddress(addr), port) {}

Endpoint Endpoint::fromString(const QString& str) {
  Endpoint endpoint;

  auto match = endpoint_regexp.match(str);
  if (!match.hasMatch()) throw EndpointNotMatched();

  return Endpoint(QHostAddress(match.captured(1)), (quint16)match.captured(2).toUInt());
}

QString Endpoint::toString() const {
  QHostAddress addr_converted = addr;
  {
    bool ok = false;
    quint32 addr4 = addr_converted.toIPv4Address(&ok);
    if (ok) addr_converted = QHostAddress(addr4);
  }

  QString endpoint_template = QStringLiteral("%1:%2");
  if (addr_converted.protocol() == QAbstractSocket::IPv6Protocol) endpoint_template = QStringLiteral("[%1]:%2");

  return endpoint_template.arg(addr_converted.toString()).arg(port);
}

Endpoint Endpoint::fromPacked4(QByteArray packed) {
  packed = packed.leftJustified(6, 0, true);

  Endpoint endpoint;
  endpoint.addr = QHostAddress(qFromBigEndian(*reinterpret_cast<quint32*>(packed.mid(0, 4).data())));
  endpoint.port = qFromBigEndian(*reinterpret_cast<quint16*>(packed.mid(4, 2).data()));
  return endpoint;
}

Endpoint Endpoint::fromPacked6(QByteArray packed) {
  packed = packed.leftJustified(18, 0, true);

  Endpoint endpoint;
  endpoint.addr = QHostAddress(reinterpret_cast<quint8*>(packed.mid(0, 16).data()));
  endpoint.port = qFromBigEndian(*reinterpret_cast<quint16*>(packed.mid(16, 2).data()));
  return endpoint;
}

EndpointList Endpoint::fromPackedList4(const QByteArray& packed) {
  EndpointList l;
  l.reserve(packed.size() / 6);
  for (int i = 0; i < packed.size(); i += 6) l.push_back(fromPacked4(packed.mid(i, 6)));
  return l;
}

EndpointList Endpoint::fromPackedList6(const QByteArray& packed) {
  EndpointList l;
  l.reserve(packed.size() / 18);
  for (int i = 0; i < packed.size(); i += 18) l.push_back(fromPacked6(packed.mid(i, 18)));
  return l;
}

Endpoint Endpoint::fromSockaddr(const sockaddr& sa) {
  quint16 port_be = 0;
  switch (sa.sa_family) {
    case AF_INET:
      port_be = reinterpret_cast<const sockaddr_in*>(&sa)->sin_port;
      break;
    case AF_INET6:
      port_be = reinterpret_cast<const sockaddr_in6*>(&sa)->sin6_port;
      break;
    default:
      throw InvalidAddressFamily();
  }
  return {QHostAddress(&sa), qFromBigEndian(port_be)};
}

std::tuple<sockaddr_storage, size_t> Endpoint::toSockaddr() const {
  sockaddr_storage sa = {};

  switch (addr.protocol()) {
    case QAbstractSocket::IPv4Protocol: {
      sockaddr_in* sa4 = (sockaddr_in*)&sa;
      sa4->sin_family = AF_INET;
      sa4->sin_port = qToBigEndian(port);
      sa4->sin_addr.s_addr = addr.toIPv4Address();
      return {sa, sizeof(sockaddr_in)};
    }
    case QAbstractSocket::IPv6Protocol: {
      sockaddr_in6* sa6 = (sockaddr_in6*)&sa;
      sa6->sin6_family = AF_INET6;
      sa6->sin6_port = qToBigEndian(port);
      Q_IPV6ADDR addr6 = addr.toIPv6Address();
      std::copy((const char*)&addr6, (const char*)&addr6 + 16, (char*)&sa6->sin6_addr);
      return {sa, sizeof(sockaddr_in6)};
    }
    default:
      return {sa, sizeof(sockaddr_storage)};
  }
}

Endpoint Endpoint::fromJson(const QJsonObject& j) {
  return Endpoint(QHostAddress(j["ip"].toString()), j["port"].toInt());
}

QJsonObject Endpoint::toJson() const { return QJsonObject{{"ip", addr.toString()}, {"port", port}}; }

QDebug operator<<(QDebug debug, const Endpoint& endpoint) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "Endpoint(" << endpoint.toString() << ")";
  return debug;
}

}  // namespace librevault
