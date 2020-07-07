#include "FolderParams.h"

#include <QJsonArray>
#include <QtDebug>

namespace librevault {

FolderParams::FolderParams(QVariantMap fconfig) {
  // Necessary
  secret = fconfig["secret"].toString();
  path = fconfig["path"].toString();

  // Optional
  system_path = fconfig["system_path"].isValid() ? fconfig["system_path"].toString() : path + "/.librevault";
  index_event_timeout = std::chrono::milliseconds(fconfig["index_event_timeout"].toInt());
  preserve_unix_attrib = fconfig["preserve_unix_attrib"].toBool();
  preserve_windows_attrib = fconfig["preserve_windows_attrib"].toBool();
  preserve_symlinks = fconfig["preserve_symlinks"].toBool();
  normalize_unicode = fconfig["normalize_unicode"].toBool();
  chunk_strong_hash_type = Meta::StrongHashType(fconfig["chunk_strong_hash_type"].toInt());
  full_rescan_interval = std::chrono::seconds(fconfig["full_rescan_interval"].toInt());

  for (const QString& ignore_path : fconfig["ignore_paths"].toStringList()) ignore_paths.push_back(ignore_path);
  for (const QString& node : fconfig["nodes"].toStringList()) nodes.push_back(node);

  QString archive_type_str = fconfig["archive_type"].toString();
  if (archive_type_str == "none") archive_type = ArchiveType::NO_ARCHIVE;
  if (archive_type_str == "trash") archive_type = ArchiveType::TRASH_ARCHIVE;
  if (archive_type_str == "timestamp") archive_type = ArchiveType::TIMESTAMP_ARCHIVE;
  if (archive_type_str == "block") archive_type = ArchiveType::BLOCK_ARCHIVE;

  archive_trash_ttl = fconfig["archive_trash_ttl"].toInt();
  archive_timestamp_count = fconfig["archive_timestamp_count"].toInt();
  mainline_dht_enabled = fconfig["mainline_dht_enabled"].toBool();
}

}  // namespace librevault
