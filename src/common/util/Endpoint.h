#pragma once
#include <QHostAddress>
#include <QJsonObject>

#include "util/exception.hpp"

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
