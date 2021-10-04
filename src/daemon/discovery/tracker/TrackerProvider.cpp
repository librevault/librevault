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
#include "TrackerProvider.h"

#include <Version.h>

#include <QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkReply>
#include <utility>

#include "nat/PortMappingService.h"
#include "nodekey/NodeKey.h"

Q_LOGGING_CATEGORY(log_tracker, "discovery.tracker")

namespace librevault {

// TrackerProvider
TrackerProvider::TrackerProvider(NodeKey* nodekey, PortMappingService* port_mapping, QNetworkAccessManager* nam,
                                 QObject* parent)
    : QObject(parent), nodekey_(nodekey), port_mapping_(port_mapping), nam_(nam) {}
TrackerProvider::~TrackerProvider() {}

QByteArray TrackerProvider::getDigest() const { return nodekey_->digest(); }

quint16 TrackerProvider::getExternalPort() { return port_mapping_->mappedPortOrOriginal("main"); }

void TrackerProvider::addGroup(const QByteArray& groupid) {
  groups_[groupid] = std::make_unique<TrackerGroup>(groupid, getDigest(), getExternalPort(), nam_, this);
  connect(groups_[groupid].get(), &TrackerGroup::discovered, this, &TrackerProvider::discovered);
  groups_[groupid]->addAnnouncer({"https://tracker.librevault.com"});
}

// TrackerGroup
TrackerGroup::TrackerGroup(QByteArray groupid, QByteArray peerid, quint16 port, QNetworkAccessManager* nam,
                           TrackerProvider* provider)
    : QObject(provider),
      nam_(nam),
      provider_(provider),
      groupid_(std::move(groupid)),
      peerid_(std::move(peerid)),
      port_(port) {}

void TrackerGroup::addAnnouncer(const QUrl& baseurl) {
  auto announcer = new TrackerAnnouncer(baseurl, nam_, this);
  connect(announcer, &TrackerAnnouncer::discovered, this,
          [=](DiscoveryResult result) { emit discovered(groupid_, result); });
  announcers_[QUuid::createUuid().toString()] = std::unique_ptr<TrackerAnnouncer>(announcer);
}

QByteArray TrackerGroup::getRequestMessage() const {
  return QJsonDocument{{{"group_id", QString::fromLatin1(groupid_.toHex())},
                        {"peer_id", QString::fromLatin1(peerid_.toHex())},
                        {"port", port_}}}
      .toJson();
}

// TrackerAnnouncer
TrackerAnnouncer::TrackerAnnouncer(const QUrl& baseurl, QNetworkAccessManager* nam, TrackerGroup* group)
    : QObject(group),
      nam_(nam),
      group_(group),
      baseurl_(baseurl),
      trackerinfo_url_(baseurl),
      announce_url_(baseurl),
      deannounce_url_(baseurl) {
  timer_ = new QTimer(this);
  connect(timer_, &QTimer::timeout, this, &TrackerAnnouncer::sendAnnounceOne);

  trackerinfo_url_.setPath(baseurl_.path() + "/v1/trackerinfo");
  announce_url_.setPath(baseurl_.path() + "/v1/announce");
  deannounce_url_.setPath(baseurl_.path() + "/v1/deannounce");

  QNetworkRequest request(trackerinfo_url_);
  request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, Version::current().userAgent());
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
  request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
  nam_->get(request);

  QTimer::singleShot(0, this, &TrackerAnnouncer::sendAnnounceOne);
  timer_->setInterval(60 * 1000);
  timer_->start();
}

void TrackerAnnouncer::sendAnnounceOne() {
  QNetworkRequest request(announce_url_);
  request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, Version::current().userAgent());
  request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
  request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

  qCDebug(log_tracker) << "Sending announce to:" << announce_url_;

  auto reply = nam_->post(request, group_->getRequestMessage());
  connect(reply, &QNetworkReply::finished, this, [=] { handleAnnounceReply(reply->readAll()); });
}

void TrackerAnnouncer::handleAnnounceReply(const QByteArray& reply) {
  qCDebug(log_tracker) << "Received response from tracker:" << reply;

  auto message = QJsonDocument::fromJson(reply);
  timer_->setInterval(message["ttl"].toInt(60) * 1000);
  for (QJsonValueRef peer : message["peers"].toArray()) {
    auto peer_obj = peer.toObject();
    DiscoveryResult result;
    result.url = QUrl(message["url"].toString());
    result.digest = QByteArray::fromHex(message["peer_id"].toString().toLatin1());
    emit discovered(result);
  }
}

}  // namespace librevault
