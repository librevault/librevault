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
#include "SHA3.h"

#include <cryptopp/sha3.h>

#include <QCryptographicHash>

namespace librevault {
namespace crypto {

blob SHA3::compute(const blob& data) const {
  switch (size_) {
    case 224:
      return conv_bytearray(QCryptographicHash::hash(conv_bytearray(data), QCryptographicHash::Sha3_224));
    case 256:
      return conv_bytearray(QCryptographicHash::hash(conv_bytearray(data), QCryptographicHash::Sha3_256));
    default:
      throw std::runtime_error("Hashing algorithm not supported");
  }
}
blob SHA3::to(const blob& data) const { return compute(data); }

}  // namespace crypto
}  // namespace librevault
