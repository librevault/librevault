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
#pragma once
#include "src/pch.h"
#include <QSystemTrayIcon>
#include <QMenu>

class Client;
class TrayIcon : public QSystemTrayIcon {
Q_OBJECT

public:
	TrayIcon(Client& client);

signals:
	void show_main_window();

private:
	Client& client_;
	std::unique_ptr<QMenu> context_menu_;

private slots:
	void open_website();
};
