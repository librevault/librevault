#pragma once
#include <QLoggingCategory>
#include <QObject>
#include <QRunnable>
#include <QString>
#include <QThreadPool>
#include <map>

#include "SignedMeta.h"

namespace librevault {

class FolderParams;
class MetaStorage;
class IgnoreList;
class PathNormalizer;

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
  Meta::Chunk populate_chunk(const QByteArray& data, const QHash<QByteArray, QByteArray>& pt_hmac__iv);
};

}  // namespace librevault
