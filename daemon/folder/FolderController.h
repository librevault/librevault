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

#include "control/FolderSettings_fwd.h"
#include "control/PersistentConfiguration.h"
#include "util/exception.hpp"
#include <QHash>
#include <QObject>

namespace librevault {

class Config;

class NodeKey;
class PeerServer;

class FolderGroup;

class BTProvider;
class DHTProvider;
class MulticastProvider;

class FolderController : public PersistentConfiguration {
  Q_OBJECT

 public:
  FolderController(Config* config, BTProvider* bt, MulticastProvider* mcast, DHTProvider* dht, PeerServer* peerserver, NodeKey* node_key, QObject* parent);

  DECLARE_EXCEPTION(samekey_error,
      "Multiple directories with the same key (or derived from the same key) are not supported");

  Q_SLOT void loadAll();
  Q_SLOT void unloadAll();

  void loadFolder(const QJsonObject &folder_settings);
  void unloadFolder(const QByteArray &folderid);

  QList<QByteArray> list() const;
  void importAll(const QJsonArray& folder_configs);
  QJsonArray exportAll() const;

 private:
  Config* config_ = nullptr;

  NodeKey* node_key_ = nullptr;
  PeerServer* peerserver_ = nullptr;

  BTProvider* bt_ = nullptr;
  DHTProvider* dht_ = nullptr;
  MulticastProvider* mcast_ = nullptr;

  QHash<QByteArray, FolderGroup*> groups_;

  bool config_imported_ = false;
};

} /* namespace librevault */
