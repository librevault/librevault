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
#include <cstdint>
#include <string>
#include <vector>
#include <QByteArray>

namespace librevault {
namespace crypto {

using blob = std::vector<uint8_t>;
inline QByteArray from_blob(const blob& b) {
  return QByteArray((const char*)b.data(), b.size());
}
inline blob to_blob(const QByteArray& b) {
  return blob(b.begin(), b.end());
}

class OneWayTransformer {
public:
	virtual ~OneWayTransformer() {}

	virtual blob to(const blob& data) const = 0;

	template <class InputIterator>
	blob to(InputIterator first, InputIterator last) const {
		return to(blob(first, last));
	}
	template <class Container>
	blob to(const Container& data) const {
		return to(data.begin(), data.end());
	}

	template <class InputIterator>
	std::string to_string(InputIterator first, InputIterator last) const {
		blob result = to(first, last);
		return std::string(std::make_move_iterator(result.begin()), std::make_move_iterator(result.end()));
	}
	template <class Container>
	std::string to_string(const Container& data) const {
		return to_string(data.begin(), data.end());
	}
};

class TwoWayTransformer : public OneWayTransformer {
public:
	virtual ~TwoWayTransformer() {}

	virtual blob to(const blob& data) const = 0;
	virtual blob from(const blob& data) const = 0;

	template <class InputIterator>
	blob from(InputIterator first, InputIterator last) const {
		return from(blob(first, last));
	}
	template <class Container>
	blob from(const Container& data) const {
		return from(data.begin(), data.end());
	}
};

template<class Trans>
class De : public TwoWayTransformer {
	Trans nested;
public:
	template<class...Args>
	De(Args... trans_args) : nested(trans_args...) {}

	blob to(const blob& data) const {return nested.from(data);};
	blob from(const blob& data) const {return nested.to(data);};
};

template <class Container>
inline blob operator|(const Container& data, OneWayTransformer&& transformer) {
	return transformer.to(data.begin(), data.end());
}

} /* namespace crypto */
} /* namespace librevault */
