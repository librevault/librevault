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
#include <QLoggingCategory>
#include <QObject>
#include <QRunnable>
#include <QThreadPool>
#include <QString>
#include <map>

#include "SignedMeta.h"

namespace librevault {

struct FolderParams;
class MetaStorage;
class IgnoreList;
class PathNormalizer;

struct MemoryView {
  const char* ptr = nullptr;
  size_t size = 0;
  [[nodiscard]] QByteArray array() const { return QByteArray(ptr, size); }
};

class IndexerWorker : public QObject, public QRunnable {
  Q_OBJECT
 signals:
  void metaCreated(SignedMeta smeta);
  void metaFailed(QString errorString);

 public:
  struct AbortIndex : public std::runtime_error {
    explicit AbortIndex(const QString& what) : std::runtime_error(what.toStdString()) {}
  };

  IndexerWorker(QString abspath, const FolderParams& params, MetaStorage* meta_storage, IgnoreList* ignore_list,
                PathNormalizer* path_normalizer, QObject* parent);
  ~IndexerWorker() override;

  [[nodiscard]] QString absolutePath() const { return abspath_; }

 public slots:
  void run() noexcept override;
  void stop() { active_ = false; };

 private:
  QString abspath_;
  const FolderParams& params_;
  MetaStorage* meta_storage_;
  IgnoreList* ignore_list_;
  PathNormalizer* path_normalizer_;

  const Secret& secret_;

  Meta old_meta_, new_meta_;
  SignedMeta old_smeta_, new_smeta_;

  /* Status */
  std::atomic<bool> active_;

  void make_Meta();

  /* File analyzers */
  Meta::Type get_type();
  void update_fsattrib();
  void update_chunks();
};

}  // namespace librevault
