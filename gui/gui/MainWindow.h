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
#pragma once
#include "gui/Settings.h"
#include "gui/AddFolder.h"
#include "gui/OpenLink.h"
#include "ui_MainWindow.h"
#include <QMainWindow>
#include <QJsonObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <memory>

class FolderModel;
class FolderProperties;
class StatusBar;

class MainWindow : public QMainWindow {
Q_OBJECT

public:
	MainWindow(Daemon* daemon, FolderModel* folder_model, Updater* updater);
	~MainWindow();

signals:
	void folderAdded(QString secret, QString path);
	void folderRemoved(QString secret);

public slots:
	void showWindow();
	void retranslateUi();
	void openWebsite();

	void handle_disconnected(QString message);
	void handle_connected();

protected slots:
	void tray_icon_activated(QSystemTrayIcon::ActivationReason reason);
	void showFolderContextMenu(const QPoint& point);
	void handleRemoveFolder();
	void handleOpenFolderProperties(const QModelIndex &index);

protected:
	/* UI elements */
	Ui::MainWindow ui;
	StatusBar* status_bar_;

	/* Models */
	FolderModel* folder_model_;

	/* Dialogs */
	Settings* settings_;
	QMap<QByteArray, FolderProperties*> folder_properties_windows_;
public:
	AddFolder* add_folder_;
	OpenLink* open_link_;

protected:
	/* Event handlers (overrides) */
	void changeEvent(QEvent* e) override;
	//void closeEvent(QCloseEvent* e) override;

	/* Actions */
	QAction* show_main_window_action;
	QAction* open_website_action;
	QAction* show_settings_window_action;
	QAction* exit_action;
	QAction* new_folder_action;
	QAction* open_link_action;
	QAction* delete_folder_action;

	void init_actions();
	void init_toolbar();

	/* Tray icon */
	QSystemTrayIcon tray_icon;
	QMenu tray_context_menu;
	QMenu folders_menu;

	void init_tray();

	/* Daemon */
	Daemon* daemon_;
};
