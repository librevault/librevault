#pragma once

#include <QtCore>

#ifdef Q_OS_UNIX

#include <QSocketNotifier>
#include <csignal>

namespace librevault {

class UnixSignalHandler : public QObject {
  Q_OBJECT
 public:
  explicit UnixSignalHandler(QObject* parent);
  ~UnixSignalHandler() override = default;

  void addSignals(const QList<int>& signal_codes = {SIGTERM, SIGINT});

  Q_SIGNAL void signalReceived(int sig);

 private:
  QSocketNotifier* signal_notifier_;

  void handleUnixSignal();
};

}  // namespace librevault

#endif
