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
#pragma once
#include <unordered_map>
#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtNetwork/QNetworkAccessManager>

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
