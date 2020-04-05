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
#include "SettingsWindowPrivate.mac.h"
#include "SettingsWindow.h"
#include <QDebug>
#include <QTimer>
#include <AppKit/NSToolbar.h>
#include <AppKit/NSToolbarItem.h>
#include <AppKit/NSImage.h>

SettingsWindowPrivate::SettingsWindowPrivate(SettingsWindow* window) : window_(window) {
	toolbar = new QMacToolBar(window_);
	window_->window()->winId();
	window_layout_ = new QVBoxLayout(window_);
	pane_layout_ = new QVBoxLayout();
	window_layout_->addLayout(pane_layout_);

	QWidget* bottom_zone = new QWidget(window_);
	window_->ui_bottom_.setupUi(bottom_zone);
	window_layout_->addWidget(bottom_zone);
}

void SettingsWindowPrivate::add_pane(QWidget* pane) {
	auto toolButton = toolbar->addItem(pane->windowIcon(), pane->windowTitle());
	int page_num = (int)buttons_.size();
	buttons_.push_back(toolButton);
	QObject::connect(toolButton, &QMacToolBarItem::activated, [=]{
		emit showPane(page_num);
	});
	toolButton->setSelectable(true);

	panes_.push_back(pane);
	pane_layout_->addWidget(pane);

	if(page_num == 0)
		showPane(0);
	else
		pane->setVisible(false);

	QTimer::singleShot(0, [=]{
		toolbar->detachFromWindow();
		toolbar->attachToWindow(window_->window()->windowHandle());
	});
}

void SettingsWindowPrivate::showPane(int page) {
	// Select in toolbar
	NSString* item_id = toolbar->items().at(page)->nativeToolBarItem().itemIdentifier;
	[toolbar->nativeToolbar() setSelectedItemIdentifier:item_id];

	// Show pane
	for(int i = 0; i < panes_.size(); i++)
		panes_[i]->setVisible(i == page);
	QTimer::singleShot(0, window_, &QWidget::adjustSize);
}
