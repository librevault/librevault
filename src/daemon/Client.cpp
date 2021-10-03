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
#include "Client.h"

#include "control/Config.h"
#include "control/StateCollector.h"
#include "control/server/ControlServer.h"
#include "discovery/Discovery.h"
#include "folder/FolderGroup.h"
#include "folder/FolderService.h"
#include "nat/PortMappingService.h"
#include "nodekey/NodeKey.h"
#include "p2p/P2PProvider.h"

namespace librevault {

Client::Client(int argc, char** argv) : QCoreApplication(argc, argv) {
  setApplicationName("Librevault");
  setOrganizationDomain("librevault.com");

#ifdef Q_OS_UNIX
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sig_fd_)) qFatal("Couldn't create TERM socketpair");
  signal_notifier_ = new QSocketNotifier(sig_fd_[1], QSocketNotifier::Read, this);
  connect(signal_notifier_, &QSocketNotifier::activated, this, [this] { handleUnixSignal(); });
#endif

  // Initializing components
  nam_ = new QNetworkAccessManager(this);
  //  nam_->setAutoDeleteReplies(true);
  //  nam_->setStrictTransportSecurityEnabled(true);
  //  nam_->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

  state_collector_ = new StateCollector(this);
  node_key_ = new NodeKey(this);
  portmanager_ = new PortMappingService(nam_, this);
  discovery_ = new Discovery(node_key_, portmanager_, state_collector_, nam_, this);
  folder_service_ = new FolderService(state_collector_, this);
  p2p_provider_ = new P2PProvider(node_key_, portmanager_, folder_service_, this);
  control_server_ = new ControlServer(state_collector_, this);

  /* Connecting signals */
  connect(state_collector_, &StateCollector::globalStateChanged, control_server_,
          &ControlServer::notify_global_state_changed);
  connect(state_collector_, &StateCollector::folderStateChanged, control_server_,
          &ControlServer::notify_folder_state_changed);

  connect(discovery_, &Discovery::discovered, p2p_provider_, &P2PProvider::handleDiscovered);

  connect(folder_service_, &FolderService::folderAdded, control_server_, [this](FolderGroup* group) {
    control_server_->notify_folder_added(group->folderid(), Config::get()->getFolder(group->folderid()));
  });
  connect(folder_service_, &FolderService::folderRemoved, control_server_,
          [this](FolderGroup* group) { control_server_->notify_folder_removed(group->folderid()); });

  connect(folder_service_, &FolderService::folderAdded, discovery_, &Discovery::addGroup);

  connect(control_server_, &ControlServer::restart, this, &Client::restart);
  connect(control_server_, &ControlServer::shutdown, this, &Client::shutdown);
}

Client::~Client() {
  delete control_server_;
  delete p2p_provider_;
  delete folder_service_;
  delete discovery_;
  delete portmanager_;
  delete node_key_;
  delete state_collector_;
  delete nam_;
}

int Client::run() {
  folder_service_->run();
  control_server_->run();

  return this->exec();
}

void Client::restart() {
  qInfo() << "Restarting...";
  this->exit(EXIT_RESTART);
}

void Client::shutdown() {
  qInfo() << "Exiting...";
  this->exit();
}

#ifdef Q_OS_UNIX
int Client::sig_fd_[2] = {};

void Client::unixSignalHandler(int sig) {
  qDebug() << "Is it handled?" << sig;
  ::write(sig_fd_[0], &sig, sizeof(sig));
}

void Client::handleUnixSignal() {
  signal_notifier_->setEnabled(false);
  int sig;
  ::read(sig_fd_[1], &sig, sizeof(sig));
  // qt actions below

  switch (sig) {
    case SIGTERM:
    case SIGINT:
      shutdown();
      break;
    default:;
  }

  // qt actions above
  signal_notifier_->setEnabled(true);
}
#endif

}  // namespace librevault
