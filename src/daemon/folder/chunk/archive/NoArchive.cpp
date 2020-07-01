#include "NoArchive.h"

#include <QFile>

namespace librevault {

void NoArchive::archive(const QString &denormpath) { QFile::remove(denormpath); }

}  // namespace librevault
