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
#include "StartupInterface.h"
#include <CoreServices/CoreServices.h>
#include <MacTypes.h>
#include <Foundation/Foundation.h>
#include <Cocoa/Cocoa.h>

struct MacStartupInterface {
	LSSharedFileListRef login_items;
	NSURL* item_url;

	LSSharedFileListItemRef getLoginItem() {
		Boolean foundIt=false;
		UInt32 seed = 0U;
		NSArray * currentLoginItems = [NSMakeCollectable(LSSharedFileListCopySnapshot(login_items, &seed))
		autorelease];
		for (id itemObject in currentLoginItems) {
			LSSharedFileListItemRef item = (LSSharedFileListItemRef) itemObject;

			UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;
			CFURLRef URL = NULL;
			OSStatus err = LSSharedFileListItemResolve(item, resolutionFlags, &URL, /*outRef*/ NULL);
			if (err == noErr) {
				foundIt = CFEqual(URL, item_url);
				CFRelease(URL);

				if (foundIt)
					return item;
			}
		}
		return nil;
	}
};

StartupInterface::StartupInterface(QObject* parent) :
		QObject(parent) {
	interface_impl_ = new MacStartupInterface;
	static_cast<MacStartupInterface*>(interface_impl_)->login_items = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
	static_cast<MacStartupInterface*>(interface_impl_)->item_url = [NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]];
	[static_cast<MacStartupInterface*>(interface_impl_)->item_url retain];
}

StartupInterface::~StartupInterface() {
	[static_cast<MacStartupInterface*>(interface_impl_)->item_url release];
	CFRelease(static_cast<MacStartupInterface*>(interface_impl_)->login_items);
	delete static_cast<MacStartupInterface*>(interface_impl_);
}

bool StartupInterface::isSupported() {
	return true;
}

bool StartupInterface::isEnabled() const {
	return static_cast<MacStartupInterface*>(interface_impl_)->getLoginItem() != nil;
}

void StartupInterface::enable() {
	auto login_item = static_cast<MacStartupInterface*>(interface_impl_)->getLoginItem();
	if(login_item == nil) {
		LSSharedFileListInsertItemURL(static_cast<MacStartupInterface *>(interface_impl_)->login_items,
									  kLSSharedFileListItemBeforeFirst,
									  NULL, NULL, (CFURLRef)(static_cast<MacStartupInterface *>(interface_impl_)->item_url), NULL, NULL);
	}
}

void StartupInterface::disable() {
	auto login_item = static_cast<MacStartupInterface*>(interface_impl_)->getLoginItem();
	if(login_item != nil) {
		LSSharedFileListItemRemove(static_cast<MacStartupInterface *>(interface_impl_)->login_items, login_item);
	}
}
