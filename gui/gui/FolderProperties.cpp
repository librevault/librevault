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
#include "FolderProperties.h"
#include "ui_FolderProperties.h"
#include <QShowEvent>

FolderProperties::FolderProperties(QWidget* parent) :
		QDialog(parent),
		ui(std::make_unique<Ui::FolderProperties>()) {
	ui->setupUi(this);

	//connect(ui->button_CreateSecret, &QPushButton::clicked, this, &FolderProperties::generateSecret);
	//connect(ui->button_Browse, &QPushButton::clicked, this, &FolderProperties::browseFolder);
	//connect(this, &QDialog::accepted, this, &AddFolder::handleAccepted);
	this->setWindowFlags(Qt::Tool);
}

FolderProperties::~FolderProperties() {}

void FolderProperties::setSecret(const librevault::Secret& secret) {
	if(secret.get_type() <= secret.ReadWrite)
		ui->secret_rw->setText(QString::fromStdString(secret.string()));
	else
		ui->secret_rw->setText("");

	if(secret.get_type() <= secret.ReadOnly)
		ui->secret_ro->setText(QString::fromStdString(secret.derive(secret.ReadOnly).string()));
	else
		ui->secret_ro->setText("");

	if(secret.get_type() <= secret.Download)
		ui->secret_do->setText(QString::fromStdString(secret.derive(secret.Download).string()));
	else
		ui->secret_do->setText("");
}
