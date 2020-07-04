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
  ~Client() override;

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
