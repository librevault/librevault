#include "ClientDaemon.h"

#include "Client.h"
#include "UnixSignalHandler.h"
#include "control/Config.h"
#include "control/StateCollector.h"
#include "control/server/ControlServer.h"

namespace librevault {

ClientDaemon::ClientDaemon(int argc, char** argv) : QCoreApplication(argc, argv) {
  setApplicationName("Librevault");
  setOrganizationDomain("librevault.com");

#ifdef Q_OS_UNIX
  sighandler_ = new UnixSignalHandler(this);
  connect(sighandler_, &UnixSignalHandler::signalReceived, this, &ClientDaemon::handleUnixSignal);
  sighandler_->addSignals({SIGTERM, SIGINT});
#endif

  client_ = new Client(this);

  // Initializing components
  control_server_ = new ControlServer(client_->stateCollector(), this);

  /* Connecting signals */
  connect(client_, &Client::globalStateChanged, control_server_, &ControlServer::notifyGlobalStateChanged);
  connect(client_, &Client::folderStateChanged, control_server_, &ControlServer::notifyFolderStateChanged);

  connect(client_, &Client::folderAdded, control_server_, [this](const QByteArray& group) {
    control_server_->notifyFolderAdded(group, Config::get()->getFolder(group));
  });
  connect(client_, &Client::folderRemoved, control_server_, &ControlServer::notifyFolderRemoved);

  connect(control_server_, &ControlServer::restart, this, &ClientDaemon::restart);
  connect(control_server_, &ControlServer::shutdown, this, &ClientDaemon::shutdown);
}

int ClientDaemon::run() {
  client_->run();
  control_server_->run();

  return QCoreApplication::exec();
}

void ClientDaemon::restart() {
  qInfo() << "Restarting...";
  QCoreApplication::exit(EXIT_RESTART);
}

void ClientDaemon::shutdown() {
  qInfo() << "Exiting...";
  QCoreApplication::exit();
}

#ifdef Q_OS_UNIX
void ClientDaemon::handleUnixSignal(int sig) {
  switch (sig) {
    case SIGTERM:
    case SIGINT:
      shutdown();
      break;
    default:;
  }
}
#endif

}  // namespace librevault
