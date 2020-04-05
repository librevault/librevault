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

namespace librevault {
namespace crypto {

class SHA3 : public OneWayTransformer {
	size_t size_;
public:
	SHA3(size_t size) : size_(size) {}
	virtual ~SHA3() {}

	blob compute(const blob& data) const;
	blob to(const blob& data) const;
};

} /* namespace crypto */
} /* namespace librevault */
