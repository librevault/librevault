#pragma once
#include <QByteArray>
#include <QCache>
#include <QMutex>
#include <QObject>

namespace librevault {

class MemoryStorage : public QObject {
  Q_OBJECT
 public:
  MemoryStorage(QObject* parent);

  bool have_chunk(const QByteArray& ct_hash) const noexcept;
  QByteArray get_chunk(const QByteArray& ct_hash) const;
  void put_chunk(const QByteArray& ct_hash, QByteArray data);

 private:
  mutable QMutex cache_lock_;
  QCache<QByteArray, QByteArray> cache_;
};

}  // namespace librevault
