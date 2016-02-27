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
#include <cryptopp/sha3.h>

namespace librevault {
namespace crypto {

/**
 * This transformation is not really HMAC, as HMAC(Keccak) is not supported by CryptoPP.
 * As described on http://keccak.noekeon.org/ (than's Keccak creators' site):
 *
 *   Unlike SHA-1 and SHA-2, Keccak does not have the length-extension weakness,
 *   hence does not need the HMAC nested construction. Instead, MAC computation
 *   can be performed by simply prepending the message with the key.
 *
 * Okay, this solution is implemented here. Just prepending key to message.
 */
class HMAC_SHA3_224 : public OneWayTransformer {
	const blob key_;
	mutable CryptoPP::SHA3_224 hasher_;
public:
	HMAC_SHA3_224(blob key) : key_(std::move(key)) {}
	virtual ~HMAC_SHA3_224() {}

	blob compute(const blob& data) const {
		blob result(hasher_.DigestSize());

		hasher_.Update((const uint8_t*)key_.data(), key_.size());
		hasher_.Update(data.data(), data.size());
		hasher_.Final(result.data());

		return result;
	}
	blob to(const blob& data) const {
		return compute(data);
	}
};

} /* namespace crypto */
} /* namespace librevault */
