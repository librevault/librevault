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
#include "Transformer.h"
#include <cryptopp/sha.h>
#include <cryptopp/hmac.h>

namespace librevault {
namespace crypto {

class HMAC_SHA2_224 : public OneWayTransformer {
	mutable CryptoPP::HMAC<CryptoPP::SHA224> hasher_;
public:
	HMAC_SHA2_224(const blob& key) : hasher(key.data(), key.size()) {}
	virtual ~HMAC_SHA2_224() {}

	blob compute(const blob& data) const {
		blob result(hasher_.DigestSize());
		hasher_.CalculateDigest(result.data(), data.data(), data.size());
		return result;
	}
	blob to(const blob& data) const {
		return compute(data);
	}
};

} /* namespace crypto */
} /* namespace librevault */
