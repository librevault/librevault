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
#include "DaemonProcess.h"
#include <QCoreApplication>
#include <QUrl>
#include <QMessageBox>
#include <QDebug>

DaemonProcess::DaemonProcess(QObject* parent) : QProcess(parent) {
	/* Process parameters */
	// Program
	setProgram(get_executable_path());
	// Arguments
	QStringList arguments;
	//arguments << QString("--data=\"") + QCoreApplication::applicationDirPath() + "\"";
	arguments << QString("-vv");
	setArguments(arguments);

	setReadChannel(StandardOutput);
}

DaemonProcess::~DaemonProcess() {
	if(state() == Running) {
		terminate();
		waitForFinished();
		kill();
	}
}

void DaemonProcess::launch() {
	connect(this, static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error), this, &DaemonProcess::handleError);
	connect(this, &QProcess::readyReadStandardOutput, this, &DaemonProcess::handleStandardOutput);
	start();
}

void DaemonProcess::handleError(QProcess::ProcessError error) {
	switch(error) {
		case FailedToStart:
			emit daemonFailed("Couldn't launch Librevault service");
			break;
		case Crashed:
			emit daemonFailed("Librevault service crashed");
			break;
		default:
			emit daemonFailed("Unknown error");
	}
}

void DaemonProcess::handleStandardOutput() {
	while(canReadLine() && !listening_already) {
		auto stdout_line = readLine();
		/* Regexp for getting listen address from STDOUT */
		QRegExp listen_regexp(R"(^\[CONTROL\].*(http:\/\/\S*))", Qt::CaseSensitive, QRegExp::RegExp2);  // It may compile several times (if not optimized by Qt)
		if(listen_regexp.indexIn(stdout_line) > -1) {
			listening_already = true;

			QUrl daemon_url = listen_regexp.cap(1);

			// Because, if listening on all interfaces, then we can connect to localhost
			if( (daemon_url.host() == "0.0.0.0") | (daemon_url.host() == "::") )
				daemon_url.setHost("localhost");

			qDebug() << "Connecting to: " << daemon_url;

			emit daemonReady(daemon_url);
		}
	}
}
