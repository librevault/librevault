#include "KMAC-SHA3.h"

#include <QCryptographicHash>

namespace librevault::crypto {

QByteArray KMAC_SHA3_224::to(const QByteArray& data) const {
  QCryptographicHash hasher(QCryptographicHash::Algorithm::Sha3_224);
  hasher.addData(key_);
  hasher.addData(data);
  return hasher.result();
}

}  // namespace librevault::crypto
