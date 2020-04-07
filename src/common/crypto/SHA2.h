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
#include <cryptopp/sha.h>

#include "Transformer.h"

namespace librevault {
namespace crypto {

class SHA2 : public OneWayTransformer {
  const size_t size;
  mutable CryptoPP::SHA224 hasher224;
  mutable CryptoPP::SHA256 hasher256;
  mutable CryptoPP::SHA384 hasher384;
  mutable CryptoPP::SHA512 hasher512;

 public:
  SHA2(size_t size) : size(size) {}
  virtual ~SHA2() {}

  blob compute(const blob& data) const {
    blob result;
    switch (size) {
      case 224: {
        result.resize(hasher224.DigestSize());
        hasher224.CalculateDigest(result.data(), data.data(), data.size());
      } break;
      case 256: {
        result.resize(hasher256.DigestSize());
        hasher256.CalculateDigest(result.data(), data.data(), data.size());
      } break;
      case 384: {
        result.resize(hasher384.DigestSize());
        hasher384.CalculateDigest(result.data(), data.data(), data.size());
      } break;
      case 512: {
        result.resize(hasher512.DigestSize());
        hasher512.CalculateDigest(result.data(), data.data(), data.size());
      } break;
    }
    return result;
  }
  blob to(const blob& data) const { return compute(data); }
};

}  // namespace crypto
}  // namespace librevault
