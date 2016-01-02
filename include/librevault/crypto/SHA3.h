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

class SHA3 : public OneWayTransformer {
	const size_t size;
	mutable CryptoPP::SHA3 hasher;
public:
	SHA3(size_t size) : size(size), hasher(size/8) {}
	virtual ~SHA3() {}

	blob compute(const blob& data) const {
		blob result(hasher.DigestSize());
		hasher.CalculateDigest(result.data(), data.data(), data.size());
		return result;
	}
	blob to(const blob& data) const {
		return compute(data);
	}
};

} /* namespace crypto */
} /* namespace librevault */
