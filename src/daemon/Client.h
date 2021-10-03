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

#define EXIT_RESTART 451

#include <QCoreApplication>
#include <QtCore/QSocketNotifier>
#include <QtNetwork/QNetworkAccessManager>
#include <memory>

namespace librevault {

/* Components */
class ControlServer;
class Discovery;
class FolderService;
class NodeKey;
class P2PProvider;
class PortMappingService;
class StateCollector;

class Client : public QCoreApplication {
  Q_OBJECT
 public:
  Client(int argc, char** argv);
  virtual ~Client();

  int run();
  void restart();
  void shutdown();

#ifdef Q_OS_UNIX
  static void unixSignalHandler(int sig);
#endif
 private:
  QNetworkAccessManager* nam_;
  StateCollector* state_collector_;
  NodeKey* node_key_;
  PortMappingService* portmanager_;
  Discovery* discovery_;
  FolderService* folder_service_;
  P2PProvider* p2p_provider_;
  ControlServer* control_server_;

#ifdef Q_OS_UNIX
  static int sig_fd_[2];
  QSocketNotifier* signal_notifier_;

  void handleUnixSignal();
#endif
};

}  // namespace librevault
