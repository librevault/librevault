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
#include "TrayIcon.h"
#include "src/Client.h"
#include <QDesktopServices>
#include <QUrl>

TrayIcon::TrayIcon(Client& client) :
		QSystemTrayIcon(), client_(client) {
	// Context menu
	context_menu_ = std::make_unique<QMenu>();
	/// Show Librevault window
	QAction* show_main_window_action = context_menu_->addAction(tr("Show Librevault window"));
	connect(show_main_window_action, &QAction::triggered, this, &TrayIcon::show_main_window);

	/// Open Librevault website
	QAction* open_website_action = context_menu_->addAction(tr("Open Librevault website"));
	connect(open_website_action, &QAction::triggered, this, &TrayIcon::open_website);

	/// -----
	context_menu_->addSeparator();

	/// Exit
	QAction* exit_action = context_menu_->addAction(tr("Exit Librevault"));
	connect(exit_action, &QAction::triggered, &client, &Client::exit);

	setContextMenu(context_menu_.get());

	// QSystemTrayIcon parameters themselves
	connect(this, &QSystemTrayIcon::activated, this, &TrayIcon::show_main_window);

	setIcon(QIcon(":/logo/favicon-v.svg"));
	show();
	setToolTip(tr("Librevault"));
}

void TrayIcon::open_website() {
	QDesktopServices::openUrl(QUrl("https://librevault.com"));
}
