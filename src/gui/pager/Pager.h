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

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QToolButton>

class Pager : public QWidget {
Q_OBJECT

public:
	Pager(QHBoxLayout* layout, QWidget* parent = 0);

signals:
	void pageSelected(int page);

public:
	int add_page();
	void set_text(int page, const QString& text);
	void set_icon(int page, const QIcon& icon);

	int page_count() {return (int)buttons_.size();}

private slots:
	void buttonClicked(int page);

private:

	QHBoxLayout* layout_;
	std::vector<QToolButton*> buttons_;
};
