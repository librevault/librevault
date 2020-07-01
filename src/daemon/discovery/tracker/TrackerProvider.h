#pragma once
#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtNetwork/QNetworkAccessManager>
#include <unordered_map>

#include "discovery/DiscoveryResult.h"

namespace librevault {

class NodeKey;
class PortMappingService;
class TrackerGroup;
class TrackerAnnouncer;

class TrackerProvider : public QObject {
  Q_OBJECT
  friend class TrackerGroup;

 signals:
  void discovered(QByteArray folderid, DiscoveryResult result);

 public:
  TrackerProvider(NodeKey* nodekey, PortMappingService* port_mapping, QNetworkAccessManager* nam, QObject* parent);
  virtual ~TrackerProvider();

  void addGroup(const QByteArray& groupid);

  [[nodiscard]] QByteArray getDigest() const;
  [[nodiscard]] quint16 getExternalPort();

 private:
  NodeKey* nodekey_;

  PortMappingService* port_mapping_;
  QNetworkAccessManager* nam_;

  std::unordered_map<QByteArray, std::unique_ptr<TrackerGroup>> groups_;
};

class TrackerGroup : public QObject {
  Q_OBJECT
  friend class TrackerAnnouncer;

 public:
  TrackerGroup(QByteArray groupid, QByteArray peerid, quint16 port, QNetworkAccessManager* nam,
               TrackerProvider* provider);

  void addAnnouncer(const QUrl& baseurl);
  [[nodiscard]] QByteArray getRequestMessage() const;

  Q_SIGNAL void discovered(QByteArray groupid, DiscoveryResult result);

 private:
  QNetworkAccessManager* nam_;
  TrackerProvider* provider_;

  const QByteArray groupid_;
  const QByteArray peerid_;
  const quint16 port_;

  std::unordered_map<QString, std::unique_ptr<TrackerAnnouncer>> announcers_;
};

class TrackerAnnouncer : public QObject {
  Q_OBJECT
 public:
  TrackerAnnouncer(const QUrl& baseurl, QNetworkAccessManager* nam, TrackerGroup* group);

  Q_SIGNAL void discovered(DiscoveryResult result);

 private:
  void sendAnnounceOne();
  void handleAnnounceReply(const QByteArray& reply);

  QTimer* timer_;
  QNetworkAccessManager* nam_;
  TrackerGroup* group_;
  QUrl baseurl_, trackerinfo_url_, announce_url_, deannounce_url_;
};

}  // namespace librevault
