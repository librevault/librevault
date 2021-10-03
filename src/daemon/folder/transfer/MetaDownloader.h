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
#include <QObject>

#include "SignedMeta.h"
#include "util/conv_bitfield.h"
#include "util/log.h"

namespace librevault {

class RemoteFolder;
class MetaStorage;
class Downloader;

class MetaDownloader : public QObject {
  Q_OBJECT
  LOG_SCOPE("MetaDownloader");

 public:
  MetaDownloader(MetaStorage* meta_storage, Downloader* downloader, QObject* parent);

  /* Message handlers */
  void handle_have_meta(RemoteFolder* origin, const Meta::PathRevision& revision, const bitfield_type& bitfield);
  void handle_meta_reply(RemoteFolder* origin, const SignedMeta& smeta, const bitfield_type& bitfield);

 private:
  MetaStorage* meta_storage_;
  Downloader* downloader_;
};

}  // namespace librevault
