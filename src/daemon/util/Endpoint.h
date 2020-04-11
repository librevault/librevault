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
#pragma once
#include <QHostAddress>
#include <QJsonObject>

#include "exception.hpp"

struct sockaddr;
struct sockaddr_storage;

namespace librevault {

struct Endpoint;  // forward declaration

using EndpointList = QList<Endpoint>;
using EndpointSet = QSet<Endpoint>;

struct Endpoint {
  DECLARE_EXCEPTION(InvalidEndpoint, "Invalid endpoint");
  DECLARE_EXCEPTION(InvalidAddressFamily, "Invalid address family inside sockaddr");
  DECLARE_EXCEPTION(EndpointNotMatched, "Endpoint has not match the endpoint pattern");

  Endpoint() = default;
  Endpoint(const QHostAddress& addr, quint16 port);
  Endpoint(const QString& addr, quint16 port);

  QHostAddress addr;
  quint16 port = 0;

  static Endpoint fromString(const QString& str);
  [[nodiscard]] QString toString() const;

  static Endpoint fromPacked4(QByteArray packed);
  static Endpoint fromPacked6(QByteArray packed);
  static EndpointList fromPackedList4(const QByteArray& packed);
  static EndpointList fromPackedList6(const QByteArray& packed);

  static Endpoint fromSockaddr(const sockaddr& sa);
  [[nodiscard]] std::tuple<sockaddr_storage, size_t> toSockaddr() const;

  [[nodiscard]] inline QPair<QHostAddress, quint16> toPair() const { return {addr, port}; };
  [[nodiscard]] inline bool operator==(const Endpoint& that) const { return toPair() == that.toPair(); }

  static Endpoint fromJson(const QJsonObject& j);
  [[nodiscard]] QJsonObject toJson() const;
};

[[maybe_unused]] inline uint qHash(const Endpoint& key, uint seed = 0) noexcept { return qHash(key.toPair(), seed); }

QDebug operator<<(QDebug debug, const Endpoint& endpoint);

}  // namespace librevault
