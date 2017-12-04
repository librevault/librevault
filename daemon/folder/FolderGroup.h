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
#include "secret/Secret.h"
#include "SignedMeta.h"
#include "config/models.h"
#include "util/BandwidthCounter.h"
#include <QLoggingCategory>
#include <QObject>
#include <memory>

namespace librevault {

Q_DECLARE_LOGGING_CATEGORY(log_folder);

class Peer;

class IgnoreList;

class MetaTaskScheduler;
class Index;
class ChunkStorage;
class DirectoryPoller;
class DirectoryWatcher;

class MetaUploader;
class MetaDownloader;
class Uploader;
class Downloader;

class PeerPool;
class NodeKey;

class FolderGroup : public QObject {
  Q_OBJECT

 public:
  FolderGroup(FolderSettings params, QObject* parent);
  virtual ~FolderGroup();

  const FolderSettings& params() const { return params_; }

  void setPeerPool(PeerPool* pool);

 private:
  const FolderSettings params_;

  // Local storage
  IgnoreList* ignore_list_ = nullptr;
  MetaTaskScheduler* task_scheduler_ = nullptr;
  Index* index_ = nullptr;
  ChunkStorage* chunk_storage_ = nullptr;
  DirectoryPoller* poller_ = nullptr;
  DirectoryWatcher* watcher_ = nullptr;

  // P2P transfers
  PeerPool* pool_ = nullptr;
  Uploader* uploader_ = nullptr;
  Downloader* downloader_ = nullptr;
  MetaUploader* meta_uploader_ = nullptr;
  MetaDownloader* meta_downloader_ = nullptr;

 private slots:
  // void handleAddedChunk(const QByteArray& ct_hash);
  void addIndexing(const QString& abspath);
  void addAssemble(SignedMeta smeta);

  void notifyLocalMeta(const SignedMeta& smeta);
  void handleNewChunk(const QByteArray& ct_hash);
  void handleNewPeer(Peer* peer);

 private:
  void createServiceDirectory();
};

} /* namespace librevault */
