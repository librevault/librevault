#pragma once
#include <QList>
#include <QString>
#include <QUrl>
#include <QVariantMap>
#include <chrono>

#include "Meta.h"
#include "Secret.h"

namespace librevault {

class FolderParams {
 public:
  enum class ArchiveType : unsigned { NO_ARCHIVE = 0, TRASH_ARCHIVE, TIMESTAMP_ARCHIVE, BLOCK_ARCHIVE };

  FolderParams(QVariantMap fconfig);

  /* Parameters */
  Secret secret;
  QString path;
  QString system_path;
  std::chrono::milliseconds index_event_timeout;
  bool preserve_unix_attrib;
  bool preserve_windows_attrib;
  bool preserve_symlinks;
  bool normalize_unicode;
  Meta::StrongHashType chunk_strong_hash_type;
  std::chrono::seconds full_rescan_interval;
  QStringList ignore_paths;
  QList<QUrl> nodes;
  ArchiveType archive_type;
  unsigned archive_trash_ttl;
  unsigned archive_timestamp_count;
  bool mainline_dht_enabled;
};

}  // namespace librevault
