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

#include <AppKit/NSToolbar.h>
#include <AppKit/NSToolbarItem.h>
#include <AppKit/NSImage.h>
#include "OSXIconInterface.h"

void set_named_image(void* nstoolbaritem, void* str) {
	NSToolbarItem* item = (NSToolbarItem*)nstoolbaritem;
	const char* c_str = (const char*)str;

	NSString *nsstr = [NSString stringWithUTF8String:c_str];
	NSImage* image = [NSImage imageNamed:nsstr];
	item.image = image;
}

void select_item(void* nstoolbar, void* nstoolbaritem) {
	NSToolbar* toolbar = (NSToolbar*)nstoolbar;
	NSToolbarItem* item = (NSToolbarItem*)nstoolbaritem;
	NSString* item_id = item.itemIdentifier;

	[toolbar setSelectedItemIdentifier:item_id];
}
