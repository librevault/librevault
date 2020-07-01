#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>
#include <memory>

#include "StartupInterface.h"

struct WindowsStartupInterface {
  std::unique_ptr<QSettings> settings;
};

StartupInterface::StartupInterface(QObject* parent) : QObject(parent) {
  interface_impl_ = new WindowsStartupInterface{};

  static_cast<WindowsStartupInterface*>(interface_impl_)->settings = std::make_unique<QSettings>(
      "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
}

StartupInterface::~StartupInterface() {}

bool StartupInterface::isSupported() { return true; }

bool StartupInterface::isEnabled() const {
  return static_cast<WindowsStartupInterface*>(interface_impl_)->settings->contains("Librevault");
}

void StartupInterface::enable() {
  QString app_path = QCoreApplication::applicationFilePath();
  app_path.replace("/", "\\");
  static_cast<WindowsStartupInterface*>(interface_impl_)->settings->setValue("Librevault", app_path);
  static_cast<WindowsStartupInterface*>(interface_impl_)->settings->sync();
}

void StartupInterface::disable() {
  static_cast<WindowsStartupInterface*>(interface_impl_)->settings->remove("Librevault");
  static_cast<WindowsStartupInterface*>(interface_impl_)->settings->sync();
}
