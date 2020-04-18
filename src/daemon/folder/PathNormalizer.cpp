/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
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
