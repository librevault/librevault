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
#include "Client.h"
#include "src/gui/MainWindow.h"
#include "src/gui/TrayIcon.h"
#include "src/model/FolderModel.h"
#include "src/control/ControlClient.h"
#include <QCommandLineParser>

Client::Client(int &argc, char **argv, int appflags) :
		QApplication(argc, argv, appflags) {
	// Parsing arguments
	QCommandLineParser parser;
	QCommandLineOption attach_option(QStringList() << "a" << "attach", tr("Attach to running daemon instead of creating a new one"), "url");
	parser.addOption(attach_option);
	parser.process(*this);

	// Creating components
	control_client_ = std::make_unique<ControlClient>(parser.value(attach_option));

	folder_model_ = std::make_unique<FolderModel>();

	main_window_ = std::make_unique<MainWindow>();
	trayicon_ = std::make_unique<TrayIcon>(*this);

	// Setting model
	main_window_->set_model(folder_model_.get());

	// Connecting signals & slots
	connect(trayicon_.get(), &TrayIcon::show_main_window, main_window_.get(), &QMainWindow::show);
	connect(control_client_.get(), &ControlClient::state_json_received, main_window_.get(), &MainWindow::handle_state_json);
	connect(control_client_.get(), &ControlClient::state_json_received, folder_model_.get(), &FolderModel::set_state_json);
}

Client::~Client() {}

void Client::exit() {
	closeAllWindows();
	quit();
}
