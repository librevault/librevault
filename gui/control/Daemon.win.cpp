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
#include "Daemon.h"
#include <QCoreApplication>
#include <windows.h>

BOOL Is64BitWindows() {
#	if defined(_WIN64)
	return TRUE;
#	elif defined(_WIN32)
	BOOL f64 = FALSE;
    return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#	else
	return FALSE;
#	endif
}


QString Daemon::get_executable_path() const {
	if(Is64BitWindows())
		return QCoreApplication::applicationDirPath() + "/x64/librevault-daemon.exe";
	else
		return QCoreApplication::applicationDirPath() + "/x32/librevault-daemon.exe";
}
