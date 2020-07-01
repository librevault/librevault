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
		NSArray* currentLoginItems = (__bridge_transfer NSArray*)LSSharedFileListCopySnapshot(login_items, &seed);
		for (id itemObject in currentLoginItems) {
			LSSharedFileListItemRef item = (__bridge LSSharedFileListItemRef)itemObject;

			UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;

			NSURL* url = (__bridge_transfer NSURL*)LSSharedFileListItemCopyResolvedURL(item, resolutionFlags, NULL);
			if(url && CFEqual((__bridge CFURLRef)url, (__bridge CFURLRef)item_url))
				return item;
		}
		return nil;
	}
};

StartupInterface::StartupInterface(QObject* parent) :
		QObject(parent) {
	interface_impl_ = new MacStartupInterface;
	static_cast<MacStartupInterface*>(interface_impl_)->login_items = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
	static_cast<MacStartupInterface*>(interface_impl_)->item_url = [NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]];
}

StartupInterface::~StartupInterface() {
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
									  NULL, NULL, (__bridge CFURLRef)(static_cast<MacStartupInterface *>(interface_impl_)->item_url), NULL, NULL);
	}
}

void StartupInterface::disable() {
	auto login_item = static_cast<MacStartupInterface*>(interface_impl_)->getLoginItem();
	if(login_item != nil) {
		LSSharedFileListItemRemove(static_cast<MacStartupInterface *>(interface_impl_)->login_items, login_item);
	}
}
