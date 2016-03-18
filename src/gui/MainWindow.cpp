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
#include "MainWindow.h"
#include "src/model/FolderModel.h"
#include "ui_MainWindow.h"
#include <QCloseEvent>
#include <QPushButton>
#include <QDesktopServices>
#include <QDebug>
#include <src/icons/GUIIconProvider.h>

#ifdef Q_OS_MAC
void qt_mac_set_dock_menu(QMenu *menu);
#endif

MainWindow::MainWindow(Client& client, QWidget* parent) :
		QMainWindow(parent),
		client_(client),
		ui(std::make_unique<Ui::MainWindow>()) {
	ui->setupUi(this);
	/* Initializing models */
	folder_model_ = std::make_unique<FolderModel>();
	set_model(folder_model_.get());

	/* Initializing dialogs */
	settings_ = std::make_unique<Settings>();
	connect(settings_.get(), &Settings::newConfigIssued, this, &MainWindow::newConfigIssued);

	add_folder_ = new AddFolder(this);
	connect(add_folder_, &AddFolder::folderAdded, this, &MainWindow::folderAdded);

	init_actions();
	init_tray();
	init_toolbar();
	//auto button = new QPushButton();
	//button->setText(tr("My Account"));
	//button->setFlat(true);
	//ui->statusBar->addPermanentWidget(button);
	retranslateUi();
}

MainWindow::~MainWindow() {}

void MainWindow::retranslateUi() {
	show_main_window_action->setText(tr("Show Librevault window"));
	open_website_action->setText(tr("Open Librevault website"));
	show_settings_window_action->setText(tr("Settings"));
	show_settings_window_action->setToolTip(tr("Open Librevault settings"));
	exit_action->setText(tr("Quit Librevault"));    // TODO: Apply: https://ux.stackexchange.com/questions/50893/do-we-exit-quit-or-close-an-application
	exit_action->setToolTip("Stop synchronization and exit Librevault application");
	new_folder_action->setText(tr("New folder"));
	new_folder_action->setToolTip(tr("Add new folder for synchronization"));
	delete_folder_action->setText(tr("Delete"));
	delete_folder_action->setToolTip(tr("Delete folder"));

	ui->retranslateUi(this);
	settings_->retranslateUi();
}

void MainWindow::handleControlJson(QJsonObject state_json) {
	settings_->handleControlJson(state_json);
	folder_model_->handleControlJson(state_json);
}

void MainWindow::openWebsite() {
	QDesktopServices::openUrl(QUrl("https://librevault.com"));
}

void MainWindow::tray_icon_activated(QSystemTrayIcon::ActivationReason reason) {
	if(reason != QSystemTrayIcon::Context) show_main_window_action->trigger();
}

void MainWindow::handleRemoveFolder() {
	auto selection_model = ui->treeView->selectionModel()->selectedRows(2);
	for(auto model_index : selection_model) {
		qDebug() << model_index;
		QString secret = folder_model_->data(model_index).toString();
		qDebug() << secret;
		emit folderRemoved(secret);
	}
}

void MainWindow::set_model(FolderModel* model) {
	ui->treeView->setModel(model);
}

void MainWindow::changeEvent(QEvent* e) {
	QMainWindow::changeEvent(e);
	switch(e->type()) {
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;
		default:
			break;
	}
}

void MainWindow::init_actions() {
	show_main_window_action = new QAction(this);
	connect(show_main_window_action, &QAction::triggered, this, &QMainWindow::show);

	open_website_action = new QAction(this);
	connect(open_website_action, &QAction::triggered, this, &MainWindow::openWebsite);

	show_settings_window_action = new QAction(this);
	show_settings_window_action->setIcon(GUIIconProvider::get_instance()->get_icon(GUIIconProvider::SETTINGS));
	show_settings_window_action->setMenuRole(QAction::PreferencesRole);
	connect(show_settings_window_action, &QAction::triggered, settings_.get(), &QDialog::show);

	exit_action = new QAction(this);
	QIcon exit_action_icon; // TODO
	exit_action->setIcon(exit_action_icon);
	connect(exit_action, &QAction::triggered, this, &QCoreApplication::quit);

	new_folder_action = new QAction(this);
	new_folder_action->setIcon(GUIIconProvider::get_instance()->get_icon(GUIIconProvider::FOLDER_ADD));
	connect(new_folder_action, &QAction::triggered, add_folder_, &AddFolder::show);

	delete_folder_action = new QAction(this);
	delete_folder_action->setIcon(GUIIconProvider::get_instance()->get_icon(GUIIconProvider::FOLDER_DELETE));
	delete_folder_action->setShortcut(Qt::Key_Delete);
	connect(delete_folder_action, &QAction::triggered, this, &MainWindow::handleRemoveFolder);
}

void MainWindow::init_toolbar() {
	ui->toolBar->addAction(new_folder_action);
	ui->toolBar->addAction(delete_folder_action);
	ui->toolBar->addSeparator();
	ui->toolBar->addAction(show_settings_window_action);
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

	tray_icon.setIcon(QIcon(":/branding/librevault_icon.svg"));   // FIXME: Temporary measure. Need to display "sync" icon here. Also, theming support.
	tray_icon.setToolTip(tr("Librevault"));

	tray_icon.show();
}
