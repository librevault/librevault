#include "UnixSignalHandler.h"

#ifdef Q_OS_UNIX

#include <sys/socket.h>
#include <unistd.h>

#include <csignal>

namespace librevault {

static int sig_fd_[2] = {};
static std::atomic_flag initialized = false;

static void nativeHandler(int sig) {
  qDebug() << "Signal received:" << sig;
  ::write(sig_fd_[0], &sig, sizeof(sig));
}

UnixSignalHandler::UnixSignalHandler(QObject* parent) : QObject(parent) {
  if (initialized.test_and_set()) qFatal("UNIX signal handler is already initialized");
  if (sig_fd_[0] == 0 || sig_fd_[1] == 0)
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sig_fd_)) qFatal("Couldn't create socketpair");
  signal_notifier_ = new QSocketNotifier(sig_fd_[1], QSocketNotifier::Read, this);
  connect(signal_notifier_, &QSocketNotifier::activated, this, [this] { handleUnixSignal(); });
}

void UnixSignalHandler::addSignals(const QList<int>& signal_codes) {
  struct sigaction sig {};

  sig.sa_handler = nativeHandler;
  sigemptyset(&sig.sa_mask);
  sig.sa_flags |= SA_RESTART;

  for (auto signal_code : signal_codes) sigaction(signal_code, &sig, nullptr);
}

void UnixSignalHandler::handleUnixSignal() {
  signal_notifier_->setEnabled(false);
  int sig;
  ::read(sig_fd_[1], &sig, sizeof(sig));
  // qt actions below
  signalReceived(sig);
  // qt actions above
  signal_notifier_->setEnabled(true);
}

}  // namespace librevault

#endif
