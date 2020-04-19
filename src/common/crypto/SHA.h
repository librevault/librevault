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
#include <QtCore/QCryptographicHash>

#include "Transformer.h"

namespace librevault::crypto {

class SHA2 : public OneWayTransformer {
  const size_t size;

 public:
  explicit SHA2(size_t size) : size(size) {}
  virtual ~SHA2() = default;

  QByteArray to(const QByteArray& data) const {
    switch (size) {
      case 224:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha224);
      case 256:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
      case 384:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha384);
      case 512:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha512);
      default:
        throw std::runtime_error("Hashing algorithm not supported");
    }
  }
};

class SHA3 : public OneWayTransformer {
  const size_t size_;

 public:
  explicit SHA3(size_t size) : size_(size) {}
  virtual ~SHA3() = default;

  QByteArray to(const QByteArray& data) const {
    switch (size_) {
      case 224:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha3_224);
      case 256:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha3_256);
      case 384:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha3_384);
      case 512:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha3_512);
      default:
        throw std::runtime_error("Hashing algorithm not supported");
    }
  }
};

}  // namespace librevault
