#if ! __has_feature(objc_arc)
#error This file must be compiled with ARC. Either turn on ARC for the project or use -fobjc-arc flag
#endif

#include "Paths.h"
#include <Foundation/Foundation.h>

namespace librevault {

QString Paths::default_appdata_path() {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *applicationSupportDirectory = [paths firstObject];

	return QString::fromNSString(applicationSupportDirectory) + "/Librevault";
}

}
