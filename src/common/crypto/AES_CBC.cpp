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

#include <util/ffi.h>
#include <librevault-rs/src/aescbc.rs.h>

#include <utility>

namespace librevault::crypto {

AES_CBC::AES_CBC(QByteArray  key, QByteArray iv) : key(std::move(key)), iv(std::move(iv)) {}

QByteArray AES_CBC::encrypt(const QByteArray& plaintext) const {
  return from_vec(encrypt_aes256(to_slice(plaintext), to_slice(key), to_slice(iv)));
}

QByteArray AES_CBC::decrypt(const QByteArray& ciphertext) const {
  return from_vec(decrypt_aes256(to_slice(ciphertext), to_slice(key), to_slice(iv)));
}

}  // namespace librevault
