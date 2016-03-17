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
#include "GUIIconProvider.h"

GUIIconProvider::GUIIconProvider() {}

QIcon GUIIconProvider::get_icon(ICON_ID id) const {
	switch(id) {
#ifdef Q_OS_MAC
		case SETTINGS_GENERAL: return QIcon(new MacIcon("NSPreferencesGeneral"));
		case SETTINGS_ACCOUNT: return QIcon(new MacIcon("NSUser"));
		case SETTINGS_NETWORK: return QIcon(new MacIcon("NSNetwork"));
		case SETTINGS_ADVANCED: return QIcon(new MacIcon("NSAdvanced"));
#endif
		case SETTINGS_GENERAL: return QIcon(":/branding/librevault_icon.svg");
		case SETTINGS_ACCOUNT: return get_shell_icon("imageres.dll", 1029);
		case SETTINGS_NETWORK: return get_shell_icon("netcenter.dll", 7);
		case SETTINGS_ADVANCED: return get_shell_icon("imageres.dll", 27);
	}
}
