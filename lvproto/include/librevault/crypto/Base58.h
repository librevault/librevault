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

namespace Base58_alphabets {
static std::string bitcoin_alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static std::string ripple_alphabet = "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";
static std::string flickr_alphabet = "123456789abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";
}

class Base58 : public TwoWayTransformer {
private:
	const std::string& current_alphabet;
public:
	Base58(const std::string& alphabet = Base58_alphabets::bitcoin_alphabet);
	blob to(const blob& data) const;
	blob from(const blob& data) const;
};

} /* namespace crypto */
} /* namespace librevault */
