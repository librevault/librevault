#include <QFileInfo>

#include "StartupInterface.h"

struct LinuxStartupInterface {
  QString desktop_file_path;
};

StartupInterface::StartupInterface(QObject* parent) : QObject(parent) {
  interface_impl_ = new LinuxStartupInterface();

  if (qEnvironmentVariableIsSet("HOME")) {
    static_cast<LinuxStartupInterface*>(interface_impl_)->desktop_file_path = qgetenv("HOME");
    static_cast<LinuxStartupInterface*>(interface_impl_)
        ->desktop_file_path.append(QStringLiteral("/.config/autostart/Librevault.desktop"));
  }
}

StartupInterface::~StartupInterface() { delete static_cast<LinuxStartupInterface*>(interface_impl_); }

bool StartupInterface::isSupported() { return true; }

bool StartupInterface::isEnabled() const {
  QFileInfo desktop_file(static_cast<LinuxStartupInterface*>(interface_impl_)->desktop_file_path);
  return desktop_file.exists() && desktop_file.isFile();
}

void StartupInterface::enable() {
  if (!isEnabled())
    QFile::copy(QStringLiteral(":/platform/Librevault.desktop"),
                static_cast<LinuxStartupInterface*>(interface_impl_)->desktop_file_path);
}

void StartupInterface::disable() {
  if (isEnabled()) QFile::remove(static_cast<LinuxStartupInterface*>(interface_impl_)->desktop_file_path);
}
