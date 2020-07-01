#include <QCoreApplication>

#include "DaemonProcess.h"

QString DaemonProcess::get_executable_path() const {
  return QCoreApplication::applicationDirPath() + "/librevault-daemon.exe";
}
