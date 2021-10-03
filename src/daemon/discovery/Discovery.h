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
#pragma once
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>

#include "DiscoveryResult.h"

namespace librevault {

class FolderGroup;

class MulticastProvider;
class MLDHTProvider;
class TrackerProvider;

class NodeKey;
class PortMappingService;
class StateCollector;

class Discovery : public QObject {
  Q_OBJECT

 signals:
  void discovered(QByteArray folderid, DiscoveryResult result);

 public:
  Discovery(NodeKey* node_key, PortMappingService* port_mapping, StateCollector* state_collector,
            QNetworkAccessManager* nam, QObject* parent);
  virtual ~Discovery();

 public slots:
  void addGroup(FolderGroup* fgroup);
  void removeGroup(const QByteArray& groupid);

 protected:
  MulticastProvider* multicast_;
  MLDHTProvider* mldht_;
  TrackerProvider* tracker_;
};

}  // namespace librevault