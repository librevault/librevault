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
#include "src/model/FolderModel.h"
#include "src/control/Daemon.h"
#include "src/control/ControlClient.h"
#include "src/single/SingleChannel.h"
#include <QCommandLineParser>
#include <QLibraryInfo>

Client::Client(int &argc, char **argv, int appflags) :
		QApplication(argc, argv, appflags) {
	setQuitOnLastWindowClosed(false);
	applyLocale(QLocale::system().name());
	// Parsing arguments
	QCommandLineParser parser;
	QCommandLineOption attach_option(QStringList() << "a" << "attach", tr("Attach to running daemon instead of creating a new one"), "url");
	parser.addOption(attach_option);
	parser.process(*this);

	// Creating components
	single_channel_ = std::make_unique<SingleChannel>();

	daemon_ = std::make_unique<Daemon>();
	control_client_ = std::make_unique<ControlClient>();
	if(parser.isSet(attach_option))
		control_client_->open(QUrl(parser.value(attach_option)));
	else {
		connect(daemon_.get(), &Daemon::daemonReady, control_client_.get(), (void(QWebSocket::*)(const QUrl&))&QWebSocket::open);
		daemon_->launch();
	}

	main_window_ = std::make_unique<MainWindow>(*this);

	updater_ = new Updater(this);

	// Connecting signals & slots
	connect(single_channel_.get(), &SingleChannel::showMainWindow, main_window_.get(), &QMainWindow::show);

	connect(control_client_.get(), &ControlClient::ControlJsonReceived, main_window_.get(), &MainWindow::handleControlJson);

	connect(main_window_.get(), &MainWindow::newConfigIssued, control_client_.get(), &ControlClient::sendConfigJson);
	connect(main_window_.get(), &MainWindow::folderAdded, control_client_.get(), &ControlClient::sendAddFolderJson);
	connect(main_window_.get(), &MainWindow::folderRemoved, control_client_.get(), &ControlClient::sendRemoveFolderJson);
}

void Client::applyLocale(QString locale) {
	qt_translator_.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	this->installTranslator(&qt_translator_);
	translator_.load(QStringLiteral(":/lang/librevault_") + locale);
	this->installTranslator(&translator_);
	//settings_->retranslateUi();
}

Client::~Client() {}
