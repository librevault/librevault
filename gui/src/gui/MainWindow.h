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

#include <QMainWindow>
#include <QJsonObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include "src/pch.h"
#include "src/Client.h"
#include "src/gui/Settings.h"
#include "src/gui/AddFolder.h"

namespace Ui {
class MainWindow;
}

class FolderModel;

class MainWindow : public QMainWindow {
Q_OBJECT

public:
	MainWindow(Client& client, QWidget* parent = 0);
	~MainWindow();

	void set_model(FolderModel* model);

signals:
	void newConfigIssued(QJsonObject config_json);
	void folderAdded(QString secret, QString path);
	void folderRemoved(QString secret);

public slots:
	void retranslateUi();
	void handleControlJson(QJsonObject state_json);
	void openWebsite();

protected slots:
	void tray_icon_activated(QSystemTrayIcon::ActivationReason reason);
	void handleRemoveFolder();

protected:
	Client& client_;
	std::unique_ptr<Ui::MainWindow> ui;

	/* Models */
	std::unique_ptr<FolderModel> folder_model_;

	/* Dialogs */
	Settings* settings_;
	AddFolder* add_folder_;

	/* Event handlers (overrides) */
	void changeEvent(QEvent* e) override;
	//void closeEvent(QCloseEvent* e) override;

	/* Actions */
	QAction* show_main_window_action;
	QAction* open_website_action;
	QAction* show_settings_window_action;
	QAction* exit_action;
	QAction* new_folder_action;
	QAction* delete_folder_action;

	void init_actions();
	void init_toolbar();

	/* Tray icon */
	QSystemTrayIcon tray_icon;
	QMenu tray_context_menu;

	void init_tray();
};
