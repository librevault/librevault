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
#include "Pager.h"

#ifndef Q_OS_MAC

Pager::Pager(QHBoxLayout* layout, QWidget* parent) : QWidget(parent), layout_(layout) {}

int Pager::add_page() {
	auto toolButton = new QToolButton(parentWidget());
	int page_num = (int)buttons_.size();
	buttons_.push_back(toolButton);

	QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(toolButton->sizePolicy().hasHeightForWidth());
	toolButton->setSizePolicy(sizePolicy);

	toolButton->setIconSize(QSize(32, 32));

	toolButton->setCheckable(true);
	toolButton->setChecked(page_num == 0);  // default
	toolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	layout_->addWidget(toolButton);
	connect(toolButton, &QToolButton::clicked, this, [=](){
		emit buttonClicked(page_num);
	});
	return page_num;
}

void Pager::set_text(int page, const QString& text) {
	buttons_[page]->setText(text);
}

void Pager::set_icon(int page, const QIcon& icon) {
	buttons_[page]->setIcon(icon);
}

int Pager::page_count() const {
	return (int)buttons_.size();
}

void Pager::show() {}

void Pager::buttonClicked(int page) {
	for(int button_idx = 0; button_idx < (int)buttons_.size(); button_idx++) {
		buttons_[button_idx]->setChecked(button_idx == page);
	}
	emit pageSelected(page);
}

#endif
