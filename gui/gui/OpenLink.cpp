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
#include "OpenLink.h"
#include "MainWindow.h"
#include <QShowEvent>
#include <QMessageBox>

OpenLink::OpenLink(QWidget* parent) :
		QDialog(parent) {
	ui.setupUi(this);
}

OpenLink::~OpenLink() {}

bool OpenLink::handleLink(QString link) {
	QUrl url(link);
	if(!url.isValid() || url.scheme() != "lvlt") {
		QMessageBox confirmation_box(
			QMessageBox::Critical,
			tr("Wrong link format"),
			tr("The link you entered is not correct. It must begin with \"lvlt:\" and contain a valid Secret."),
			QMessageBox::Close,
			this
		);
		confirmation_box.exec();
		return false;
	}
	dynamic_cast<MainWindow*>(parent())->add_folder_->showWithSecret(url.path());

	return true;
}

void OpenLink::showEvent(QShowEvent* e) {
	ui.line_url->clear();
	adjustSize();
	e->accept();
}

void OpenLink::accept() {
	if(handleLink(ui.line_url->text()) && isVisible())
		QDialog::accept();
}
