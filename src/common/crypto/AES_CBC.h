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
#pragma once
#include <util/random.h>

#include "Transformer.h"

namespace librevault::crypto {

class AES_CBC : public TwoWayTransformer {
 private:
  const QByteArray key;
  const QByteArray iv;

 public:
  AES_CBC(const QByteArray& key, const QByteArray& iv, bool padding = true);
  ~AES_CBC() override = default;

  QByteArray encrypt(const QByteArray& plaintext) const;
  QByteArray decrypt(const QByteArray& ciphertext) const;

  static QByteArray randomIv() { return randomBytes(16); }

  QByteArray to(const QByteArray& data) const { return encrypt(data); }
  QByteArray from(const QByteArray& data) const { return decrypt(data); }
};

}  // namespace librevault
