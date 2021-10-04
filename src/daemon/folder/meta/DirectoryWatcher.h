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
#include <QThread>
#include <boost/asio/io_service.hpp>
#include <dir_monitor/dir_monitor.hpp>
#include <set>

#include "Meta.h"
#include "util/log.h"

namespace librevault {

struct FolderParams;
class IgnoreList;
class PathNormalizer;

class DirectoryWatcherThread : public QThread {
  Q_OBJECT
 signals:
  void dirEvent(boost::asio::dir_monitor_event ev);

 public:
  DirectoryWatcherThread(QString abspath, QObject* parent);
  ~DirectoryWatcherThread();

 protected:
  boost::asio::io_service monitor_ios_;  // Yes, we have a new thread for each directory, because several dir_monitors
                                         // on a single io_service behave strangely:
  boost::asio::dir_monitor monitor_;     // https://github.com/berkus/dir_monitor/issues/42

  void run() override;

  void monitorLoop();
};

class DirectoryWatcher : public QObject {
  Q_OBJECT
 signals:
  void newPath(QString abspath);

 public:
  DirectoryWatcher(const FolderParams& params, IgnoreList* ignore_list, PathNormalizer* path_normalizer,
                   QObject* parent);
  virtual ~DirectoryWatcher();

  // A VERY DIRTY HACK
  void prepareAssemble(QByteArray normpath, Meta::Type type, bool with_removal = false);

 private:
  const FolderParams& params_;
  IgnoreList* ignore_list_;
  PathNormalizer* path_normalizer_;

  DirectoryWatcherThread* watcher_thread_;

  std::multiset<QString> prepared_assemble_;

 private slots:
  void handleDirEvent(boost::asio::dir_monitor_event ev);
};

}  // namespace librevault
