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
#include "GUIIconProvider.h"

#include <QFileIconProvider>

QIcon GUIIconProvider::icon(ICON_ID id) const {
  switch (id) {
    case SETTINGS_GENERAL:
      return QIcon(":/branding/librevault_icon.svg");
    case SETTINGS_ACCOUNT:
      return /*QIcon::fromTheme("user-identity");*/ QIcon(":/icons/User-96.png");
    case SETTINGS_NETWORK:
      return /*QIcon::fromTheme("network-wireless");*/ QIcon(":/icons/Network-96.png");
    case SETTINGS_ADVANCED:
      return /*QIcon::fromTheme("document-properties");*/ QIcon(":/icons/Settings-96.png");

    case FOLDER_ADD:
      return QIcon(":/icons/Add Folder-96.png");  //"folder-new"
    case FOLDER_DELETE:
      return QIcon(":/icons/Delete-96.png");  //"edit-delete"
    case LINK_OPEN:
      return QIcon(":/icons/Add Link-96.png");
    case SETTINGS:
      return QIcon(":/icons/Settings-96.png");  //"preferences-system"
                                                //"application-exit"

/* "Reveal in Finder" without icon, others will have an icon */
#if !defined(Q_OS_MAC)
    case REVEAL_FOLDER:
      return QFileIconProvider().icon(QFileIconProvider::Folder);
#endif

/* Tray icon */
#if defined(Q_OS_MAC)
    case TRAYICON: {
      QIcon icon(":/branding/librevault_icon_black.svg");
      icon.setIsMask(true);
      return icon;
    }
#else
    case TRAYICON:
      return QIcon(":/branding/librevault_icon.svg");
#endif
    default:
      return QIcon();
  }
}
