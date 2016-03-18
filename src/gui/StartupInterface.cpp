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
#include <QFileInfo>

StartupInterface::StartupInterface(QObject* parent) :
		QObject(parent) {
#ifdef Q_OS_LINUX
	if(qEnvironmentVariableIsSet("HOME")) {
		desktop_file_path = qgetenv("HOME");
		desktop_file_path.append(QStringLiteral("/.config/autostart/Librevault.desktop"));
	}
#endif
}

StartupInterface::~StartupInterface() {}

bool StartupInterface::isSupported() {
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MAC)
	return true;
#endif
	return false;
}

bool StartupInterface::isEnabled() const {
#ifdef Q_OS_LINUX
	QFileInfo desktop_file(desktop_file_path);
	return desktop_file.exists() && desktop_file.isFile();
#endif
	return false;
}

void StartupInterface::setEnabled(bool enabled) {
	if(enabled)
		enable();
	else
		disable();
}

void StartupInterface::enable() {
#ifdef Q_OS_LINUX
	if(!isEnabled())
		QFile::copy(QStringLiteral(":/platform/Librevault.desktop"), desktop_file_path);
#endif
}

void StartupInterface::disable() {
#ifdef Q_OS_LINUX
	if(isEnabled())
		QFile::remove(desktop_file_path);
#endif
}
