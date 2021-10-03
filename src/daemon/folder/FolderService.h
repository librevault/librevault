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
#include <QMap>
#include <QObject>

#include "util/blob.h"
#include "util/log.h"

Q_DECLARE_LOGGING_CATEGORY(log_folderservice)

namespace librevault {

/* Folder info */
class FolderGroup;
class FolderParams;
class StateCollector;

class FolderService : public QObject {
  Q_OBJECT
  LOG_SCOPE("FolderService");

 public:
  struct invalid_group : std::runtime_error {
    invalid_group() : std::runtime_error("Folder group not found") {}
  };

  explicit FolderService(StateCollector* state_collector, QObject* parent);
  virtual ~FolderService();

  void run();
  void stop();

  /* FolderGroup nanagenent */
  void initFolder(const FolderParams& params);
  void deinitFolder(const QByteArray& folderid);

  FolderGroup* getGroup(const QByteArray& folderid);

 signals:
  void folderAdded(FolderGroup* fgroup);
  void folderRemoved(FolderGroup* fgroup);

 private:
  StateCollector* state_collector_;

  QMap<QByteArray, FolderGroup*> groups_;
};

}  // namespace librevault
