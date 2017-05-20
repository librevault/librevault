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
#include "CliApplication.h"
#include "appver.h"
#include "blob.h"
#include "Secret.h"
#include <QDebug>
#include <QtGlobal>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIODevice>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace librevault {

CliApplication::CliApplication(int argc, char** argv, const char* USAGE) : QCoreApplication(argc, argv) {
	args = docopt::docopt(USAGE, {argv + 1, argv + argc}, true, LV_APPVER);
	QTimer::singleShot(0, this, [this]{performProcessing();});
}

void CliApplication::performProcessing() {
	// Secret generation
	if(args["secret"].asBool()) {
		if(args["generate"].asBool()) {
			std::cout << Secret() << std::endl;
		}else if(args["derive"].asBool()) {
			std::cout << Secret(QString::fromStdString(args["<secret>"].asString())).derive((Secret::Type)args["<type>"].asString().at(0)) << std::endl;
		}else if(args["folderid"].asBool()) {
			std::cout << Secret(QString::fromStdString(args["<secret>"].asString())).getHash().toHex().toStdString() << std::endl;
		}
		quit();
		return;
	}

	// Okay, time to initialize HTTP engine
	if(args["--daemon"].isString())
		daemon_control_ = QUrl(QString::fromStdString(args["--daemon"].asString()));
	else
		daemon_control_ = QUrl(QStringLiteral("http://[::1]:42346"));

	if(daemon_control_.isEmpty() || !daemon_control_.isValid()) {
		qFatal("Daemon URL is invalid");
	}else{
		//qDebug() << "Daemon URL: " << daemon_control_;
	}

	// Initializing QNAM
	nam_ = new QNetworkAccessManager(this);

	if(args["shutdown"].asBool()) {
		action_shutdown();
	}else if(args["restart"].asBool()) {
		action_restart();
	}else if(args["version"].asBool()) {
		action_version();
	}else if(args["globals"].asBool()) {
		if(args["list"].asBool()) {
			action_globals_list();
		}else if(args["get"].asBool()) {
			action_globals_get();
		}else if(args["set"].asBool()) {
			action_globals_set();
		}else if(args["unset"].asBool()) {
			action_globals_unset();
		}
	}
}

void CliApplication::action_shutdown(){
	QNetworkRequest request(daemon_control_.toString().append("/v1/shutdown"));
	QNetworkReply* reply = nam_->post(request, QByteArray());
	connect(reply, &QNetworkReply::finished, this, &QCoreApplication::quit);
}

void CliApplication::action_restart(){
	QNetworkRequest request(daemon_control_.toString().append("/v1/restart"));
	QNetworkReply* reply = nam_->post(request, QByteArray());
	connect(reply, &QNetworkReply::finished, this, &QCoreApplication::quit);
}

void CliApplication::action_version() {
	QNetworkRequest request(daemon_control_.toString().append("/v1/version"));
	QNetworkReply* reply = nam_->get(request);
	connect(reply, &QNetworkReply::finished, [reply] {
		qStdOut() << tr("Daemon version:\t%1\n").arg(QString::fromLatin1(reply->readAll()));
		qStdOut() << tr("CLI version:\t%1\n").arg(QStringLiteral(LV_APPVER));
		quit();
	});
}

void CliApplication::action_globals_list() {
	QNetworkRequest request(daemon_control_.toString().append("/v1/globals"));
	QNetworkReply* reply = nam_->get(request);
	connect(reply, &QNetworkReply::finished, [reply] {
		qStdOut() << QJsonDocument::fromJson(reply->readAll()).toJson();
		quit();
	});
}

void CliApplication::action_globals_get() {
	QNetworkRequest request(daemon_control_.toString().append("/v1/globals/").append(QString::fromStdString(args["<key>"].asString())));
	QNetworkReply* reply = nam_->get(request);
	connect(reply, &QNetworkReply::finished, [reply] {
		qStdOut() << reply->readAll();
		quit();
	});
}

void CliApplication::action_globals_set() {
	QNetworkRequest request(daemon_control_.toString().append("/v1/globals/").append(QString::fromStdString(args["<key>"].asString())));
	QNetworkReply* reply = nam_->put(request, QString::fromStdString(args["<value>"].asString()).toUtf8());
	connect(reply, &QNetworkReply::finished, this, &QCoreApplication::quit);
}

void CliApplication::action_globals_unset() {
	QNetworkRequest request(daemon_control_.toString().append("/v1/globals/").append(QString::fromStdString(args["<key>"].asString())));
	QNetworkReply* reply = nam_->deleteResource(request);
	connect(reply, &QNetworkReply::finished, this, &QCoreApplication::quit);
}

} /* namespace librevault */
