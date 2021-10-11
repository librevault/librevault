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
 */
#include "PathNormalizer.h"

#include "control/FolderParams.h"

namespace librevault {

PathNormalizer::PathNormalizer(const FolderParams& params, QObject* parent) : QObject(parent), params_(params) {}

QByteArray PathNormalizer::normalizePath(const QString& abspath) {
  return from_rust(path_normalize(abspath.toUtf8(), params_.path.toUtf8(), params_.normalize_unicode));
}

QString PathNormalizer::denormalizePath(const QByteArray& normpath) {
  return from_rust(path_denormalize(normpath, params_.path.toUtf8()));
}

}  // namespace librevault
