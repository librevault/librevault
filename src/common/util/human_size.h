#pragma once

#include <QtCore>

inline QString human_size(uintmax_t size) {
  qreal num = size;

  if (num < 1024.0) return QCoreApplication::translate("Human Size", "%n bytes", 0, size);
  num /= 1024.0;

  if (num < 1024.0) return QCoreApplication::translate("Human Size", "%1 KB").arg(num, 0, 'f', 0);
  num /= 1024.0;

  if (num < 1024.0) return QCoreApplication::translate("Human Size", "%1 MB").arg(num, 0, 'f', 2);
  num /= 1024.0;

  if (num < 1024.0) return QCoreApplication::translate("Human Size", "%1 GB").arg(num, 0, 'f', 2);
  num /= 1024.0;

  return QCoreApplication::translate("Human Size", "%1 TB").arg(num, 0, 'f', 2);
}

inline QString human_bandwidth(qreal bandwidth) {
  if (bandwidth < 1024.0) return QCoreApplication::translate("Human Bandwidth", "%1 B/s").arg(bandwidth, 0, 'f', 0);
  bandwidth /= 1024.0;

  if (bandwidth < 1024.0) return QCoreApplication::translate("Human Bandwidth", "%1 KB/s").arg(bandwidth, 0, 'f', 1);
  bandwidth /= 1024.0;

  if (bandwidth < 1024.0) return QCoreApplication::translate("Human Bandwidth", "%1 MB/s").arg(bandwidth, 0, 'f', 1);
  bandwidth /= 1024.0;

  if (bandwidth < 1024.0) return QCoreApplication::translate("Human Bandwidth", "%1 GB/s").arg(bandwidth, 0, 'f', 1);
  bandwidth /= 1024.0;

  return QCoreApplication::translate("Human Bandwidth", "%1 TB/s").arg(bandwidth, 0, 'f', 1);
}
