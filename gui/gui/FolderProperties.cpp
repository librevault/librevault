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
#include <QFileIconProvider>
#include <QJsonArray>
#include <QClipboard>
#include <util/human_size.h>

#include "../model/PeerModel.h"

FolderProperties::FolderProperties(const librevault::Secret& secret, QWidget* parent) :
		QDialog(parent),
		ui(std::make_unique<Ui::FolderProperties>()) {

	ui->setupUi(this);

	peer_model_ = std::make_unique<PeerModel>(this);
	ui->peers_treeView->setModel(peer_model_.get());

	setSecret(secret);

	connect(ui->copy_rw, &QAbstractButton::clicked, [this](){QApplication::clipboard()->setText(ui->secret_rw->text());});
	connect(ui->copy_ro, &QAbstractButton::clicked, [this](){QApplication::clipboard()->setText(ui->secret_ro->text());});
	connect(ui->copy_do, &QAbstractButton::clicked, [this](){QApplication::clipboard()->setText(ui->secret_do->text());});

	this->setWindowFlags(Qt::Tool);
	ui->folder_icon->setPixmap(QFileIconProvider().icon(QFileIconProvider::Folder).pixmap(QSize(32, 32)));
	setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
}

FolderProperties::~FolderProperties() {}

void FolderProperties::setSecret(const librevault::Secret& secret) {
	hash_.setRawData((const char*)secret.get_Hash().data(), secret.get_Hash().size());

	if(secret.get_type() <= secret.ReadWrite)
		ui->secret_rw->setText(QString::fromStdString(secret.string()));
	else {
		ui->label_rw->setVisible(false);
		ui->secret_rw->setVisible(false);
		ui->copy_rw->setVisible(false);
	}

	if(secret.get_type() <= secret.ReadOnly)
		ui->secret_ro->setText(QString::fromStdString(secret.derive(secret.ReadOnly).string()));
	else {
		ui->label_ro->setVisible(false);
		ui->secret_ro->setVisible(false);
		ui->copy_ro->setVisible(false);
	}

	ui->secret_do->setText(QString::fromStdString(secret.derive(secret.Download).string()));
}

void FolderProperties::update(const QJsonObject& control_json, const QJsonObject& folder_config_json, const QJsonObject& folder_state_json) {
	ui->folder_name->setText(folder_config_json["path"].toString());
	ui->folder_icon->setPixmap(QFileIconProvider().icon(QFileIconProvider::Folder).pixmap(32, 32));

	ui->folder_size->setText(tr("%n file(s)", "", folder_state_json["file_count"].toInt()) + " " + human_size(folder_state_json["byte_size"].toDouble()));
	ui->connected_counter->setText(tr("%n connected", "", folder_state_json["peers"].toArray().size()));
}
