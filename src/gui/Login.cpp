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
#include "Login.h"
#include "ui_Login.h"
#include <QUrlQuery>

Login::Login(QWidget* parent) :
		QDialog(parent),
		ui(std::make_unique<Ui::Login>()) {
	ui->setupUi(this);
}

Login::~Login() {}

void Login::display() {
	this->setModal(true);
	connect(ui->webView, &QWebView::urlChanged, this, &Login::urlChanged);
	QDialog::show();
}

void Login::urlChanged(const QUrl& url) {
	if(url.host() == "127.0.0.1") {
		QUrlQuery fragment(url.fragment());
		qDebug() << "access_token = " << fragment.queryItemValue("access_token");
		qDebug() << "token_type = " << fragment.queryItemValue("token_type");
		qDebug() << "secrets_key = " << fragment.queryItemValue("secrets_key");
		//std::clog << url;
	}
}
