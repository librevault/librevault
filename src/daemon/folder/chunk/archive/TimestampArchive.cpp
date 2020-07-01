#include "TimestampArchive.h"

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <QTimer>
#include <boost/filesystem.hpp>

#include "control/FolderParams.h"
#include "folder/PathNormalizer.h"
#include "util/conv_fspath.h"
#include "util/regex_escape.h"

namespace librevault {

TimestampArchive::TimestampArchive(const FolderParams& params, PathNormalizer* path_normalizer, QObject* parent)
    : ArchiveStrategy(parent),
      params_(params),
      path_normalizer_(path_normalizer),
      archive_path_(params_.system_path + "archive") {
  QDir().mkpath(archive_path_);
}

void TimestampArchive::archive(const QString& denormpath) {
  boost::filesystem::path denormpath_fs = conv_fspath(denormpath);
  // Add a new entry
  QString archived_path = archive_path_ + "/" + path_normalizer_->normalizePath(denormpath);

  qint64 mtime = boost::filesystem::last_write_time(denormpath_fs);
  QString suffix = "~" + QDateTime::fromMSecsSinceEpoch(mtime * 1000).toString("yyyyMMdd-HHmmss");

  QFileInfo archived_info(archived_path);
  QString timestamped_path = archived_info.baseName();
  timestamped_path += suffix;
  timestamped_path += archived_info.completeSuffix();
  QFile::rename(denormpath, timestamped_path);

  // Remove
  QMap<QString, boost::filesystem::path> paths;
  QRegularExpression timestamp_regex(regex_escape(archived_info.baseName()) + R"((~\d{8}-\d{6}))" +
                                     regex_escape(archived_info.completeSuffix()));
  for (auto it = boost::filesystem::directory_iterator(denormpath_fs.parent_path());
       it != boost::filesystem::directory_iterator(); it++) {
    QRegularExpressionMatch match = timestamp_regex.match(conv_fspath(it->path()));
    if (match.hasMatch()) paths.insert(match.captured(1), denormpath_fs);
  }
  if (paths.size() > (int)params_.archive_timestamp_count && params_.archive_timestamp_count != 0) {
    boost::filesystem::remove(paths.first());
  }
}

}  // namespace librevault
