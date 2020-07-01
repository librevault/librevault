#pragma once
#include <QProcess>

class DaemonProcess : public QProcess {
  Q_OBJECT

 public:
  DaemonProcess(QObject* parent);
  ~DaemonProcess();

  void launch();

 signals:
  void daemonReady(const QUrl& control_url);
  void daemonFailed(QString reason);

 private slots:
  void handleError(QProcess::ProcessError error);
  void handleStandardOutput();

 protected:
  bool listening_already = false;
  QString get_executable_path() const;
};
