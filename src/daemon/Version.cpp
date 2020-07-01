#include "Version.h"

#include "appver.h"

namespace librevault {

Version::Version() : name_(QStringLiteral(LV_APPNAME)), version_string_(QStringLiteral(LV_APPVER)) {}

}  // namespace librevault
