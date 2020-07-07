#pragma once

#define EXIT_RESTART 451

#include <QCoreApplication>
#include <QtCore/QSocketNotifier>
#include <QtNetwork/QNetworkAccessManager>
#include <memory>

namespace librevault {

/* Components */
class ControlServer;
class Client;
class UnixSignalHandler;

class ClientDaemon : public QCoreApplication {
  Q_OBJECT
 public:
  ClientDaemon(int argc, char** argv);
  ~ClientDaemon() override = default;

  int run();
  void restart();
  void shutdown();

 private:
  Client* client_;
  ControlServer* control_server_;

#ifdef Q_OS_UNIX
  UnixSignalHandler* sighandler_;
  void handleUnixSignal(int sig);
#endif
};

}  // namespace librevault
