#pragma once
#include <QFile>
#include <QReadWriteLock>
#include <memory>

#include "util/blob.h"
#include "util/log.h"

namespace librevault {

class FolderParams;
class EncStorage : public QObject {
  Q_OBJECT
  LOG_SCOPE("EncStorage");

 public:
  EncStorage(const FolderParams& params, QObject* parent);

  bool have_chunk(const QByteArray& ct_hash) const noexcept;
  QByteArray get_chunk(const QByteArray& ct_hash) const;
  void put_chunk(const QByteArray& ct_hash, QFile* chunk_f);
  void remove_chunk(const QByteArray& ct_hash);

 private:
  const FolderParams& params_;
  mutable QReadWriteLock storage_mtx_;

  QString make_chunk_ct_name(const QByteArray& ct_hash) const noexcept;
  QString make_chunk_ct_path(const QByteArray& ct_hash) const noexcept;
};

}  // namespace librevault
