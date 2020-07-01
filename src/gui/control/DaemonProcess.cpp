#include "DaemonProcess.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>
#include <QUrl>

DaemonProcess::DaemonProcess(QObject* parent) : QProcess(parent) {
  /* Process parameters */
  // Program
  setProgram(get_executable_path());
  // Arguments
  QStringList arguments;
  // arguments << QString("--data=\"") + QCoreApplication::applicationDirPath() + "\"";
  arguments << QString("-vv");
  setArguments(arguments);

  setReadChannel(StandardOutput);
}

DaemonProcess::~DaemonProcess() {
  if (state() == Running) {
    terminate();
    waitForFinished();
    kill();
  }
}

void DaemonProcess::launch() {
  connect(this, &QProcess::errorOccurred, this, &DaemonProcess::handleError);
  connect(this, &QProcess::readyReadStandardOutput, this, &DaemonProcess::handleStandardOutput);
  start();
}

void DaemonProcess::handleError(QProcess::ProcessError error) {
  switch (error) {
    case FailedToStart:
      emit daemonFailed("Couldn't launch Librevault service");
      break;
    case Crashed:
      emit daemonFailed("Librevault service crashed");
      break;
    default:
      emit daemonFailed("Unknown error");
  }
}

void DaemonProcess::handleStandardOutput() {
  while (canReadLine() && !listening_already) {
    auto stdout_line = readLine();
    /* Regexp for getting listen address from STDOUT */
    QRegExp listen_regexp(R"(^\[CONTROL\].*(http:\/\/\S*))", Qt::CaseSensitive,
                          QRegExp::RegExp2);  // It may compile several times (if not optimized by Qt)
    if (listen_regexp.indexIn(stdout_line) > -1) {
      listening_already = true;

      QUrl daemon_url = listen_regexp.cap(1);

      // Because, if listening on all interfaces, then we can connect to localhost
      if ((daemon_url.host() == "0.0.0.0") | (daemon_url.host() == "::")) daemon_url.setHost("localhost");

      qDebug() << "Connecting to: " << daemon_url;

      emit daemonReady(daemon_url);
    }
  }
}
