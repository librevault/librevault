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
#include "Base58.h"

#include <librevaultrs.hpp>

namespace librevault::crypto {

QByteArray Base58::to(const QByteArray& data) const {
  return from_rust(b58_encode(from_cpp(data)));
}

QByteArray Base58::from(const QByteArray& data) const {
  return from_rust(b58_decode(from_cpp(data)));
}

}  // namespace librevault
