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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "GUIIconProvider.h"

GUIIconProvider::GUIIconProvider() {}

QIcon GUIIconProvider::get_icon(ICON_ID id) const {
	switch(id) {
#if defined(Q_OS_MAC)
		case SETTINGS_GENERAL: return QIcon(new MacIcon("NSPreferencesGeneral"));
		case SETTINGS_ACCOUNT: return QIcon(new MacIcon("NSUser"));
		case SETTINGS_NETWORK: return QIcon(new MacIcon("NSNetwork"));
		case SETTINGS_ADVANCED: return QIcon(new MacIcon("NSAdvanced"));
#elif defined(Q_OS_WIN)
		case SETTINGS_GENERAL: return QIcon(":/branding/librevault_icon.svg");
		case SETTINGS_ACCOUNT: return get_shell_icon(L"imageres.dll", 1029);
		case SETTINGS_NETWORK: return get_shell_icon(L"netcenter.dll", 7);
		case SETTINGS_ADVANCED: return get_shell_icon(L"imageres.dll", 27);
#elif defined(Q_OS_LINUX)
		case SETTINGS_GENERAL: return QIcon(":/branding/librevault_icon.svg");
		case SETTINGS_ACCOUNT: return /*QIcon::fromTheme("user-identity");*/QIcon(":/icons/User-96.png");
		case SETTINGS_NETWORK: return /*QIcon::fromTheme("network-wireless");*/QIcon(":/icons/Network-96.png");
		case SETTINGS_ADVANCED: return /*QIcon::fromTheme("document-properties");*/QIcon(":/icons/Settings-96.png");
#endif
		case FOLDER_ADD: return QIcon(":/icons/Add Folder-96.png"); //"folder-new"
		case FOLDER_DELETE: return QIcon(":/icons/Delete-96.png");  //"edit-delete"
		case LINK_OPEN: return QIcon(":/icons/Add Link-96.png");
		case SETTINGS: return QIcon(":/icons/Settings-96.png");     //"preferences-system"
		//"application-exit"

#if defined(Q_OS_MAC)
		case TRAYICON: {
			QIcon icon(":/branding/librevault_icon_black.svg");
#	if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
			icon.setIsMask(true);
#	endif
			return icon;
		}
#else
		case TRAYICON: return QIcon(":/branding/librevault_icon.svg");
#endif
	}
	return QIcon();
}
