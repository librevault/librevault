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
#include "Client.h"
#include "gui/MainWindow.h"
#include "model/FolderModel.h"
#include "control/ControlClient.h"
#include "single/SingleChannel.h"
#include <QCommandLineParser>
#include <QLibraryInfo>

Client::Client(int &argc, char **argv, int appflags) :
		QApplication(argc, argv, appflags) {
	setAttribute(Qt::AA_UseHighDpiPixmaps);

	setQuitOnLastWindowClosed(false);
	applyLocale(QLocale::system().name());
	// Parsing arguments
	QCommandLineParser parser;
	QCommandLineOption attach_option(QStringList() << "a" << "attach", tr("Attach to running daemon instead of creating a new one."), "url");
	parser.addOption(attach_option);
	parser.addPositionalArgument("url", tr("The \"lvlt:\" URL to open."));
	parser.process(*this);

	QString link;

	if(parser.positionalArguments().size() > 0){
		QRegularExpression link_regex("lvlt:.*");
		auto link_index = parser.positionalArguments().indexOf(link_regex);
		if(link_index != -1)
			link = parser.positionalArguments().at(link_index);
	}

	// Creating components
	single_channel_ = std::make_unique<SingleChannel>(link);

	control_client_ = std::make_unique<ControlClient>(parser.value(attach_option));

	updater_ = new Updater(this);

	main_window_ = std::make_unique<MainWindow>(*this);

	// Connecting signals & slots
	connect(single_channel_.get(), &SingleChannel::showMainWindow, main_window_.get(), &QMainWindow::show);
	connect(single_channel_.get(), &SingleChannel::openLink, this, &Client::openLink);

	connect(control_client_.get(), &ControlClient::ControlJsonReceived, main_window_.get(), &MainWindow::handleControlJson);

	connect(main_window_.get(), &MainWindow::newConfigIssued, control_client_.get(), &ControlClient::sendConfigJson);
	connect(main_window_.get(), &MainWindow::folderAdded, control_client_.get(), &ControlClient::sendAddFolderJson);
	connect(main_window_.get(), &MainWindow::folderRemoved, control_client_.get(), &ControlClient::sendRemoveFolderJson);

	// Initialization complete!
	control_client_->start();

	if(!link.isEmpty())
		openLink(link);
}

Client::~Client() {
	main_window_.reset();
	control_client_.reset();
}

bool Client::event(QEvent* event) {
	if(event->type() == QEvent::FileOpen) {
		QFileOpenEvent* open_event = static_cast<QFileOpenEvent*>(event);
		openLink(open_event->url().toString());
	}
	return QApplication::event(event);
}

void Client::applyLocale(QString locale) {
	qt_translator_.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	this->installTranslator(&qt_translator_);
	translator_.load(QStringLiteral(":/lang/librevault_") + locale);
	this->installTranslator(&translator_);
	//settings_->retranslateUi();
}

void Client::openLink(QString link) {
	main_window_->show();   // Cause, this dialog looks ugly without a visible main window
	main_window_->open_link_->handleLink(link);
}
