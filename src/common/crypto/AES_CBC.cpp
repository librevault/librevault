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
#include "AES_CBC.h"

namespace librevault::crypto {

AES_CBC::AES_CBC(const QByteArray& key, const QByteArray& iv, bool padding) : key(key), iv(iv) {}

QByteArray AES_CBC::encrypt(const QByteArray& plaintext) const {
  return from_rust(encrypt_aes256(from_cpp(plaintext), from_cpp(key), from_cpp(iv)));
}

QByteArray AES_CBC::decrypt(const QByteArray& ciphertext) const {
  return from_rust(decrypt_aes256(from_cpp(ciphertext), from_cpp(key), from_cpp(iv)));
}

}  // namespace librevault
