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
#pragma once
#include "Archive.h"

namespace librevault {

class TimestampArchive : public ArchiveStrategy {
  Q_OBJECT
 public:
  TimestampArchive(const FolderParams& params, PathNormalizer* path_normalizer, QObject* parent);
  void archive(const QString& denormpath) override;

 private:
  const FolderParams& params_;
  PathNormalizer* path_normalizer_;

  QString archive_path_;
};

}  // namespace librevault
