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
#include "ui_AddFolder.h"
#include <librevault/Secret.h>
#include <QFileDialog>
#include <QShowEvent>

AddFolder::AddFolder(QWidget* parent) :
		QDialog(parent),
		ui(std::make_unique<Ui::AddFolder>()) {
	ui->setupUi(this);

	connect(ui->button_CreateSecret, &QPushButton::clicked, this, &AddFolder::generateSecret);
	connect(ui->button_Browse, &QPushButton::clicked, this, &AddFolder::browseFolder);
	connect(this, &QDialog::accepted, this, &AddFolder::handleAccepted);
	connect(this, &QDialog::rejected, this, &AddFolder::handleRejected);
}

AddFolder::~AddFolder() {}

void AddFolder::showWithSecret(QString secret) {
	ui->line_Secret->setText(secret);
	show();
}

void AddFolder::handleAccepted() {
	auto secret = ui->line_Secret->text();
	auto path = ui->line_Folder->text();
	ui->line_Folder->clear();
	ui->line_Secret->clear();
	emit folderAdded(secret, path);
}

void AddFolder::handleRejected() {
	ui->line_Folder->clear();
	ui->line_Secret->clear();
}

void AddFolder::showEvent(QShowEvent* e) {
	adjustSize();
	QDialog::showEvent(e);
}

void AddFolder::generateSecret() {
	librevault::Secret s;
	ui->line_Secret->setText(QString::fromStdString(s.string()));
}

void AddFolder::browseFolder() {
	QString dir = QFileDialog::getExistingDirectory(this,
		tr("Choose the directory to sync"),
		QString(),
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
	);
	ui->line_Folder->setText(dir);
}
