/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
