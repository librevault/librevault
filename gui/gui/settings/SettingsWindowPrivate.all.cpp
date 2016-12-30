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
#include "SettingsWindowPrivate.all.h"
#include "SettingsWindow.h"

SettingsWindowPrivate::SettingsWindowPrivate(SettingsWindow* window) : window_(window) {
	window_layout_ = new QVBoxLayout(window_);

	button_layout_ = new QHBoxLayout();
	window_layout_->addLayout(button_layout_);

	panes_stacked_ = new QStackedWidget(window_);
	window_layout_->addWidget(panes_stacked_);

	QWidget* bottom_zone = new QWidget(window_);
	window_->ui_bottom_.setupUi(bottom_zone);
	window_layout_->addWidget(bottom_zone);
}

void SettingsWindowPrivate::add_pane(QWidget* pane) {
	auto toolButton = new QToolButton(button_layout_->widget());
	int page_num = buttons_.size();
	buttons_.push_back(toolButton);
	panes_stacked_->addWidget(pane);

	QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
	sizePolicy.setHeightForWidth(toolButton->sizePolicy().hasHeightForWidth());
	toolButton->setSizePolicy(sizePolicy);

	toolButton->setIconSize(QSize(32, 32));

	toolButton->setIcon(pane->windowIcon());
	QObject::connect(pane, &QWidget::windowIconChanged, toolButton, &QToolButton::setIcon);
	toolButton->setText(pane->windowTitle());
	QObject::connect(pane, &QWidget::windowTitleChanged, toolButton, &QToolButton::setText);

	toolButton->setCheckable(true);
	toolButton->setChecked(page_num == 0);  // default
	toolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	button_layout_->addWidget(toolButton);
	QObject::connect(toolButton, &QToolButton::clicked, [=]{buttonClicked(page_num);});
}

void SettingsWindowPrivate::buttonClicked(int pane) {
	for(int button_idx = 0; button_idx < (int)buttons_.size(); button_idx++)
		buttons_[button_idx]->setChecked(button_idx == pane);
	panes_stacked_->setCurrentIndex(pane);
}
