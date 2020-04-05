/* Written in 2015 by Alexander Shishenko <alex@shishenko.com>
 *
 * LVCrypto - Cryptographic wrapper, used in Librevault.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#include "HMAC-SHA3.h"
#include <cryptopp/sha3.h>
#include <QCryptographicHash>

namespace librevault {
namespace crypto {

blob HMAC_SHA3_224::compute(const blob& data) const {
  QCryptographicHash hasher(QCryptographicHash::Algorithm::Sha3_224);
  hasher.addData(key_);
  hasher.addData(conv_bytearray(data));
  return conv_bytearray(hasher.result());
}

QByteArray HMAC_SHA3_224::compute(const QByteArray& data) const {
    QCryptographicHash hasher(QCryptographicHash::Algorithm::Sha3_224);
    hasher.addData(key_);
    hasher.addData(data);
    return hasher.result();
}

blob HMAC_SHA3_224::to(const blob& data) const {
	return compute(data);
}

} /* namespace crypto */
} /* namespace librevault */
