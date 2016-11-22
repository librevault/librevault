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
#include "MainWindow.h"
#include "control/Daemon.h"
#include "control/RemoteConfig.h"
#include "control/RemoteState.h"
#include "gui/FolderProperties.h"
#include "gui/StatusBar.h"
#include "icons/GUIIconProvider.h"
#include "model/FolderModel.h"

#ifdef Q_OS_MAC
void qt_mac_set_dock_menu(QMenu *menu);
#endif

MainWindow::MainWindow(Daemon* daemon, FolderModel* folder_model, Updater* updater) :
		QMainWindow(), folder_model_(folder_model), daemon_(daemon) {
	/* Initializing UI */
	ui.setupUi(this);
	status_bar_ = new StatusBar(ui.statusBar, daemon_);

	/* Initializing models */
	ui.treeView->setModel(folder_model_);
	ui.treeView->header()->setStretchLastSection(false);
	ui.treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);

	/* Initializing dialogs */
	settings_ = new Settings(daemon_, updater, this);

	add_folder_ = new AddFolder(this);
	connect(add_folder_, &AddFolder::folderAdded, this, &MainWindow::folderAdded);

	open_link_ = new OpenLink(this);

	connect(ui.treeView, &QAbstractItemView::doubleClicked, this, &MainWindow::handleOpenFolderProperties);

	init_actions();
	init_tray();
	init_toolbar();

	folders_menu.addAction(folder_destination_action);
	folders_menu.addSeparator();
	folders_menu.addAction(delete_folder_action);
	folders_menu.addSeparator();
	folders_menu.addAction(folder_properties_action);
	connect(ui.treeView, &QWidget::customContextMenuRequested, this, &MainWindow::showFolderContextMenu);

	retranslateUi();
}

MainWindow::~MainWindow() {}

/* public slots */
void MainWindow::showWindow() {
	show();
	activateWindow();
	raise();
}

void MainWindow::retranslateUi() {
	show_main_window_action->setText(tr("Show Librevault window"));
	open_website_action->setText(tr("Open Librevault website"));
	show_settings_window_action->setText(tr("Settings"));
	show_settings_window_action->setToolTip(tr("Open Librevault settings"));
#if Q_OS_WIN    // ux.stackexchange.com/q/50893
	exit_action->setText(tr("Quit Librevault"));
#else
	exit_action->setText(tr("Exit"));
#endif
	exit_action->setToolTip("Stop synchronization and exit Librevault application");
	new_folder_action->setText(tr("Add folder"));
	new_folder_action->setToolTip(tr("Add new folder for synchronization"));
	open_link_action->setText(tr("Open URL"));
	open_link_action->setToolTip(tr("Open shared link"));
	delete_folder_action->setText(tr("Remove"));
	delete_folder_action->setToolTip(tr("Stop synchronization and remove folder"));
	folder_properties_action->setText(tr("Properties"));
	folder_properties_action->setToolTip(tr("Open folder properies"));

#if Q_OS_WIN
	folder_destination_action->setText(tr("Open in Explorer"));
#elif Q_OS_MAC
	folder_destination_action->setText(tr("Open in Finder"));
#else
	folder_destination_action->setText(tr("Open in file manager"));
#endif
	folder_destination_action->setToolTip(tr("Open destination folder"));

	ui.retranslateUi(this);
	settings_->retranslateUi();
}

void MainWindow::openWebsite() {
	QDesktopServices::openUrl(QUrl("https://librevault.com"));
}

void MainWindow::handle_disconnected(QString message) {
	QMessageBox msg(QMessageBox::Critical,
		tr("Problem running Librevault application"),
		tr("Problem running Librevault service: %1").arg(message));
	msg.exec();
}

void MainWindow::handle_connected() {
	//
}

/* protected slots */
void MainWindow::showFolderContextMenu(const QPoint& point) {
	folders_menu.exec(ui.treeView->mapToGlobal(point));
}

void MainWindow::tray_icon_activated(QSystemTrayIcon::ActivationReason reason) {
#ifndef Q_OS_MAC
	if(reason != QSystemTrayIcon::Context) show_main_window_action->trigger();
#endif
}

void MainWindow::handleRemoveFolder() {
	auto selection_model = ui.treeView->selectionModel()->selectedRows();
	if(selection_model.empty())
		return;

	QMessageBox confirmation_box(
		QMessageBox::Warning,
		tr("Remove folder from Librevault?"),
		tr("This folder will be removed from Librevault and no longer synced with other peers. Existing folder contents will not be altered."),
		QMessageBox::Ok | QMessageBox::Cancel,
		this
	);

	confirmation_box.setWindowModality(Qt::WindowModal);

	if(confirmation_box.exec() == QMessageBox::Ok) {
		for(auto model_index : selection_model) {
			QByteArray secret = folder_model_->data(model_index, FolderModel::HashRole).toByteArray();
			//qDebug() << secret;
			//emit folderRemoved(secret);
		}
	}
}

void MainWindow::handleOpenFolderProperties() {
	auto selection_model = ui.treeView->selectionModel()->selectedRows();
	if(selection_model.empty())
		return;

	for(auto model_index : selection_model) {
		QByteArray folderid = folder_model_->data(model_index, FolderModel::HashRole).toByteArray();

		auto it = folder_properties_windows_.find(folderid);
		if(it == folder_properties_windows_.end()) {
			auto folder_properties_window = new FolderProperties(folderid, daemon_, folder_model_, this);
			folder_properties_windows_[folderid] = folder_properties_window;
			connect(folder_properties_window, &QObject::destroyed, [this, folderid]{folder_properties_windows_.remove(folderid);});
		}else{
			it.value()->raise();
		}
	}
}

void MainWindow::handleOpenDestinationFolder() {
	auto selection_model = ui.treeView->selectionModel()->selectedRows();
	if(selection_model.empty())
		return;

	for(auto model_index : selection_model) {
		QByteArray folderid = folder_model_->data(model_index, FolderModel::HashRole).toByteArray();

		QString path = daemon_->config()->getFolderValue(folderid, "path").toString();
		if(!path.isEmpty())
			QDesktopServices::openUrl(QUrl::fromLocalFile(path));
	}
}

void MainWindow::changeEvent(QEvent* e) {
	QMainWindow::changeEvent(e);
	switch(e->type()) {
		case QEvent::LanguageChange:
			ui.retranslateUi(this);
			break;
		default:
			break;
	}
}

void MainWindow::init_actions() {
	show_main_window_action = new QAction(this);
	connect(show_main_window_action, &QAction::triggered, this, &MainWindow::showWindow);

	open_website_action = new QAction(this);
	connect(open_website_action, &QAction::triggered, this, &MainWindow::openWebsite);

	show_settings_window_action = new QAction(this);
	show_settings_window_action->setIcon(GUIIconProvider::get_instance()->get_icon(GUIIconProvider::SETTINGS));
	show_settings_window_action->setMenuRole(QAction::PreferencesRole);
	connect(show_settings_window_action, &QAction::triggered, settings_, &QDialog::show);

	exit_action = new QAction(this);
	QIcon exit_action_icon; // TODO
	exit_action->setIcon(exit_action_icon);
	connect(exit_action, &QAction::triggered, this, &QCoreApplication::quit);

	new_folder_action = new QAction(this);
	new_folder_action->setIcon(GUIIconProvider::get_instance()->get_icon(GUIIconProvider::FOLDER_ADD));
	connect(new_folder_action, &QAction::triggered, add_folder_, &AddFolder::show);

	open_link_action = new QAction(this);
	open_link_action->setIcon(GUIIconProvider::get_instance()->get_icon(GUIIconProvider::LINK_OPEN));
	connect(open_link_action, &QAction::triggered, open_link_, &AddFolder::show);

	delete_folder_action = new QAction(this);
	delete_folder_action->setIcon(GUIIconProvider::get_instance()->get_icon(GUIIconProvider::FOLDER_DELETE));
	delete_folder_action->setShortcut(Qt::Key_Delete);
	connect(ui.treeView->selectionModel(), &QItemSelectionModel::selectionChanged, [this]{delete_folder_action->setEnabled(ui.treeView->selectionModel()->hasSelection());});
	connect(delete_folder_action, &QAction::triggered, this, &MainWindow::handleRemoveFolder);

	folder_properties_action = new QAction(this);
	//folder_properties_action->setIcon(GUIIconProvider::get_instance()->get_icon(GUIIconProvider::FOLDER_DELETE));
	connect(ui.treeView->selectionModel(), &QItemSelectionModel::selectionChanged, [this]{folder_properties_action->setEnabled(ui.treeView->selectionModel()->hasSelection());});
	connect(folder_properties_action, &QAction::triggered, this, &MainWindow::handleOpenFolderProperties);

	folder_destination_action = new QAction(this);
	folder_destination_action->setIcon(QFileIconProvider().icon(QFileIconProvider::Folder));
	connect(ui.treeView->selectionModel(), &QItemSelectionModel::selectionChanged, [this]{folder_destination_action->setEnabled(ui.treeView->selectionModel()->hasSelection());});
	connect(folder_destination_action, &QAction::triggered, this, &MainWindow::handleOpenDestinationFolder);
}

void MainWindow::init_toolbar() {
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
	ui.toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#endif
	ui.toolBar->addAction(new_folder_action);
	ui.toolBar->addAction(open_link_action);
	ui.toolBar->addAction(delete_folder_action);
	ui.toolBar->addSeparator();
	ui.toolBar->addAction(show_settings_window_action);
}

void MainWindow::init_tray() {
	// Context menu
	tray_context_menu.addAction(show_main_window_action);
	tray_context_menu.addAction(open_website_action);
	tray_context_menu.addSeparator();
	tray_context_menu.addAction(show_settings_window_action);
	tray_context_menu.addSeparator();
	tray_context_menu.addAction(exit_action);
#ifdef Q_OS_MAC
	qt_mac_set_dock_menu(&tray_context_menu);
#endif
	tray_icon.setContextMenu(&tray_context_menu);

	// Icon itself
	connect(&tray_icon, &QSystemTrayIcon::activated, this, &MainWindow::tray_icon_activated);

	tray_icon.setIcon(GUIIconProvider::get_instance()->get_icon(GUIIconProvider::TRAYICON));   // FIXME: Temporary measure. Need to display "sync" icon here. Also, theming support.
	tray_icon.setToolTip(tr("Librevault"));

	tray_icon.show();
}
