#pragma once
#include <QObject>

#include "SignedMeta.h"
#include "util/SQLiteWrapper.h"
#include "util/log.h"

namespace librevault {

class FolderParams;
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
  SignedMeta getMetaByPathId(const QByteArray& path_id);
  QList<SignedMeta> getAllMeta();
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

  QList<SignedMeta> queryMeta(const std::string& sql,
                              const std::map<QString, SQLValue>& values = std::map<QString, SQLValue>());
  void wipe();

  void notifyState();
};

}  // namespace librevault
