#include <QString>

#include "DaemonProcess.h"

QString DaemonProcess::get_executable_path() const { return QStringLiteral("librevault-daemon"); }
