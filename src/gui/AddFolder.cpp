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
}

AddFolder::~AddFolder() {}

void AddFolder::handleAccepted() {
	auto secret = ui->line_Secret->text();
	auto path = ui->line_Folder->text();
	emit folderAdded(secret, path);
}

void AddFolder::showEvent(QShowEvent* e) {
	QDialog::showEvent(e);

	ui->line_Folder->clear();
	ui->line_Secret->clear();
	ui->line_AddToCloudName->clear();

	ui->radio_SecretManual->toggle();
	ui->box_AddToCloud->setChecked(false);
	ui->group_AddToCloud->setVisible(false);
	ui->stackedWidget->setCurrentIndex(0);
	adjustSize();
}

void AddFolder::generateSecret() {
	librevault::Secret s;
	ui->line_Secret->setText(QString::fromStdString(s.string()));
}

void AddFolder::browseFolder() {
	QString dir = QFileDialog::getExistingDirectory(this,
		tr("Choose directory to sync"),
		QString(),
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
	);
	ui->line_Folder->setText(dir);
}
