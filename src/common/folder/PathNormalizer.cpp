#include "PathNormalizer.h"

#include <QDir>

#include "control/FolderParams.h"

namespace librevault {

PathNormalizer::PathNormalizer(const FolderParams& params) : params_(params) {}

QByteArray PathNormalizer::normalizePath(const QString& abspath) {
  QDir root_dir(params_.path);

  // Make it relative to root
  QString normpath = root_dir.relativeFilePath(QDir::cleanPath(abspath));

  // Convert directory separators
  normpath = QDir::fromNativeSeparators(normpath);

  // Apply NFC-normalization (if not explicitly disabled for somewhat reasons)
  if (params_.normalize_unicode)  // Unicode normalization NFC (for compatibility)
    normpath = normpath.normalized(QString::NormalizationForm_C);

  // Removing last '/' in directories
  if (normpath.endsWith('/')) normpath.chop(1);

  // Convert to UTF-8
  return normpath.toUtf8();
}

QString PathNormalizer::denormalizePath(const QByteArray& normpath) {
  QDir root_dir(params_.path);

  // Convert from UTF-8
  QString denormpath = QString::fromUtf8(normpath);

  // Make it absolute
  denormpath = root_dir.absoluteFilePath(denormpath);

  // Use Mac-NFD normalization on macOS (weird Unicode 3.2 edition)
#ifdef Q_OS_MAC
  denormpath = denormpath.normalized(QString::NormalizationForm_D, QChar::Unicode_3_2);
#endif

  return denormpath;
}

}  // namespace librevault
