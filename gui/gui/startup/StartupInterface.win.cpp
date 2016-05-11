/* Copyright (C) 2015-2016 Alexander Shishenko <GamePad64@gmail.com>
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
#include "StartupInterface.h"
#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>

struct WindowsStartupInterface {
	std::unique_ptr<QSettings> settings;
};

StartupInterface::StartupInterface(QObject* parent) :
		QObject(parent) {
	interface_impl_ = new WindowsStartupInterface{};

	static_cast<WindowsStartupInterface*>(interface_impl_)->settings = std::make_unique<QSettings>("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
}

StartupInterface::~StartupInterface() {}

bool StartupInterface::isSupported() {
	return true;
}

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
