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
#include "AddFolder.h"
#include "control/Daemon.h"
#include "control/RemoteConfig.h"
#include "Secret.h"
#include <QFileDialog>
#include <QJsonObject>
#include <QShowEvent>

AddFolder::AddFolder(QString secret, Daemon* daemon, QWidget* parent) :
		QDialog(parent), daemon_(daemon) {
	setAttribute(Qt::WA_DeleteOnClose);
	ui.setupUi(this);

	ui.line_Secret->setText(secret);

	connect(ui.button_CreateSecret, &QPushButton::clicked, this, &AddFolder::generateSecret);
	connect(ui.button_Browse, &QPushButton::clicked, this, &AddFolder::browseFolder);
}

AddFolder::~AddFolder() {}

void AddFolder::accept() {
	auto secret = ui.line_Secret->text();
	auto path = ui.line_Folder->text();

	QVariantMap folder_config;
	folder_config["secret"] = secret;
	folder_config["path"] = path;
	daemon_->config()->addFolder(folder_config);

	QDialog::accept();
}

void AddFolder::showEvent(QShowEvent* e) {
	adjustSize();
	QDialog::showEvent(e);
}

void AddFolder::generateSecret() {
	librevault::Secret s;
	ui.line_Secret->setText(QString::fromStdString(s.string()));
}

void AddFolder::browseFolder() {
	QString dir = QFileDialog::getExistingDirectory(this,
		tr("Choose the directory to sync"),
		QString(),
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
	);
	ui.line_Folder->setText(dir);
}
