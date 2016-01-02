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
#include <cryptopp/crc.h>
#include <cstdint>
#include <string>

namespace librevault {
namespace crypto {

class CRC32 : public OneWayTransformer {
	mutable CryptoPP::CRC32 hasher;
public:
	virtual ~CRC32() {}

	uint32_t compute(const blob& data) const {
		uint32_t crc32;
		hasher.CalculateDigest(reinterpret_cast<uint8_t*>(&crc32), data.data(), data.size());
		return crc32;
	}
	virtual blob to(const blob& data) const {
		blob result(32/8);
		hasher.CalculateDigest(result.data(), data.data(), data.size());
		return result;
	}
};

} /* namespace crypto */
} /* namespace librevault */
