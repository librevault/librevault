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
#include "Daemon.h"
#include <QCoreApplication>
#include <QUrl>
#include <QMessageBox>
#include <QDebug>

Daemon::Daemon() :
		QProcess() {
	/* Process parameters */
	// Program

	setProgram(get_executable_path());
	// Arguments
	QStringList arguments;
	//arguments << QString("--data=\"") + QCoreApplication::applicationDirPath() + "\"";
	setArguments(arguments);

	setReadChannel(StandardOutput);
}

Daemon::~Daemon() {
	if(state() == Running) {
		terminate();
		waitForFinished();
		kill();
	}
}

void Daemon::launch() {
	connect(this, static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error), this, &Daemon::handleError);
	connect(this, &QProcess::readyReadStandardOutput, this, &Daemon::handleStandardOutput);
	start();
}

void Daemon::handleError(QProcess::ProcessError error) {
	if(error == FailedToStart) {
		QMessageBox msg(QMessageBox::Critical,
		                tr("Couldn't launch Librevault application"),
		                tr("There is a problem launching Librevault service: couldn't find \"librevault\" executable in application or system folder."));
		msg.exec();
		throw;
	}
}

void Daemon::handleStandardOutput() {
	while(!listening_already && canReadLine()) {
		auto stdout_line = readLine();
		/* Regexp for getting listen address from STDOUT */
		QRegExp listen_regexp(R"(^\[CONTROL\].*(wss?:\/\/\S*))", Qt::CaseSensitive, QRegExp::RegExp2);  // It may compile several times (if not optimized by Qt)
		if(listen_regexp.indexIn(stdout_line) > -1) {
			listening_already = true;
			emit daemonReady(QUrl(listen_regexp.cap(1)));
		}
	}
}

QString Daemon::get_executable_path() const {
	QString application_path;
#if defined(Q_OS_WIN)
	if(Is64BitWindows())
		return QCoreApplication::applicationDirPath() + "/librevault-x64.exe";
	else
		return QCoreApplication::applicationDirPath() + "/librevault-x32.exe";
#elif defined(Q_OS_MAC)
	application_path = QCoreApplication::applicationDirPath() + "/librevault";
#else
	application_path = QStringLiteral("librevault");
#endif // Q_OS_MAC
	return application_path;
}

#ifdef Q_OS_WIN
BOOL Daemon::Is64BitWindows() const {
#	if defined(_WIN64)
	return TRUE;
#	elif defined(_WIN32)
    BOOL f64 = FALSE;
    return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#	else
	return FALSE;
#	endif
}
#endif
