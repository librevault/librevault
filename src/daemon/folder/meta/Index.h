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

#include "SignedMeta.h"
#include "util/SQLiteWrapper.h"
#include "util/log.h"

#include <librevault_util/src/index.rs.h>

namespace librevault {

struct FolderParams;
class StateCollector;

class Index : public QObject {
  Q_OBJECT
  LOG_SCOPE("Index");
 signals:
  void metaAdded(SignedMeta meta);
  void metaAddedExternal(SignedMeta meta);

 public:
  Index(const FolderParams& params, StateCollector* state_collector, QObject* parent);

  /* Meta manipulators */
  bool haveMeta(const Meta::PathRevision& path_revision) noexcept;
  SignedMeta getMeta(const Meta::PathRevision& path_revision);
  SignedMeta getMeta(const QByteArray& path_id);
  QList<SignedMeta> getMeta();
  QList<SignedMeta> getExistingMeta();
  QList<SignedMeta> getIncompleteMeta();
  void putMeta(const SignedMeta& signed_meta, bool fully_assembled = false);

  bool putAllowed(const Meta::PathRevision& path_revision) noexcept;

  void setAssembled(const QByteArray& path_id);
  bool isAssembledChunk(const QByteArray& ct_hash);
  QPair<quint32, QByteArray> getChunkSizeIv(const QByteArray& ct_hash);

  /* Properties */
  QList<SignedMeta> containingChunk(const QByteArray& ct_hash);

 private:
  const FolderParams& params_;
  StateCollector* state_collector_;

  // Better use SOCI library ( https://github.com/SOCI/soci ). My "reinvented wheel" isn't stable enough.
  std::unique_ptr<SQLiteDB> db_;
  void wipe();

  void notifyState();

  rust::Box<bridge::Index> index_;
};

}  // namespace librevault
