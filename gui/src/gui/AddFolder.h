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
#pragma once

#include <QDialog>
#include "src/pch.h"

namespace Ui {
class AddFolder;
}

class AddFolder : public QDialog {
Q_OBJECT

public:
	explicit AddFolder(QWidget* parent = 0);
	~AddFolder();

signals:
	void folderAdded(QString secret, QString path);

public slots:
	void handleAccepted();

protected:
	std::unique_ptr<Ui::AddFolder> ui;

	// Overrides
	void showEvent(QShowEvent* e) override;

private slots:
	void generateSecret();
	void browseFolder();
	//void validateSecret();
};
