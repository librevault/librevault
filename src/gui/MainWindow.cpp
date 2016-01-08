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
#include "MainWindow.h"
#include "src/model/FolderModel.h"
#include "ui_MainWindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget* parent) :
		QMainWindow(parent),
		ui(std::make_unique<Ui::MainWindow>()) {
	ui->setupUi(this);
}

MainWindow::~MainWindow() {}

void MainWindow::handle_state_json(QJsonObject state_json) {
	QJsonDocument json_doc;
	json_doc.setObject(state_json);
	QString json_str = json_doc.toJson();
	ui->state_display->setText(json_str);
}

void MainWindow::set_model(FolderModel* model) {
	ui->treeView->setModel(model);

}

void MainWindow::changeEvent(QEvent* e) {
	QMainWindow::changeEvent(e);
	switch(e->type()) {
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;
		default:
			break;
	}
}

void MainWindow::closeEvent(QCloseEvent* e) {
	hide();
	e->ignore();
}
