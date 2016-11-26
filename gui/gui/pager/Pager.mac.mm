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

#include <AppKit/NSToolbar.h>
#include <AppKit/NSToolbarItem.h>
#include <AppKit/NSImage.h>

Pager::Pager(QHBoxLayout* layout, QWidget* parent) : QWidget(parent) {
	toolbar = new QMacToolBar(this);
}

int Pager::add_page() {
	auto toolButton = toolbar->addItem(QIcon(), QString());
	int page_num = (int)buttons_.size();
	buttons_.push_back(toolButton);
		connect(toolButton, &QMacToolBarItem::activated, this, [=](){
		emit buttonClicked(page_num);
	});
	toolButton->setSelectable(true);
	return page_num;
}

void Pager::set_text(int page, const QString& text) {
	buttons_[page]->setText(text);
}

void Pager::set_icon(int page, const QIcon& icon) {
	if(!icon.name().isEmpty()) {
		NSString *nsstr = [NSString stringWithUTF8String:icon.name().toUtf8().data()];
		buttons_[page]->nativeToolBarItem().image = [NSImage imageNamed:nsstr];
	}else
		buttons_[page]->setIcon(icon);
}

int Pager::page_count() const {
	return (int)buttons_.size();
}

void Pager::show() {
	NSString* item_id = buttons_[0]->nativeToolBarItem().itemIdentifier;
	[toolbar->nativeToolbar() setSelectedItemIdentifier:item_id];

	parentWidget()->window()->winId();
	toolbar->attachToWindow(parentWidget()->window()->windowHandle());
}

void Pager::buttonClicked(int page) {
	emit pageSelected(page);
}
