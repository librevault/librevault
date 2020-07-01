#pragma once
#include <QObject>
#include <boost/filesystem/path.hpp>

#include "util/log.h"

namespace librevault {

class FolderParams;
class MetaStorage;
class PathNormalizer;

struct ArchiveStrategy : public QObject {
  Q_OBJECT
 public:
  virtual void archive(const QString& denormpath) = 0;

 protected:
  ArchiveStrategy(QObject* parent) : QObject(parent) {}
};

class Archive : public QObject {
  Q_OBJECT
  LOG_SCOPE("Archive");

 public:
  Archive(const FolderParams& params, MetaStorage* meta_storage, PathNormalizer* path_normalizer, QObject* parent);

  bool archive(const QString& denormpath);

 private:
  MetaStorage* meta_storage_;
  PathNormalizer* path_normalizer_;

  ArchiveStrategy* archive_strategy_;
};

}  // namespace librevault
