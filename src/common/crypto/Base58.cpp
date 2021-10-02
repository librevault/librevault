#include "Base58.h"

#include <cryptopp/integer.h>

#include <boost/range/adaptor/reversed.hpp>
#include <utility>
#include "util/blob.h"

namespace librevault::crypto {

Base58::Base58(QByteArray alphabet) : current_alphabet(std::move(alphabet)) {}

QByteArray Base58::to(const QByteArray& data) const {
  CryptoPP::Integer int_data = conv_integer(data);

  QByteArray result;
  result.reserve(data.size() * 138 / 100 + 1);

  while (int_data > 0) {
    CryptoPP::word mod;
    CryptoPP::Integer div;

    CryptoPP::Integer::Divide(mod, div, int_data, 58);
    result += current_alphabet[int(mod)];
    int_data = div;
  }

  for (auto it = data.begin(); it != data.end() && *it == 0; it++) result += current_alphabet[0];

  std::reverse(result.begin(), result.end());
  return result;
}

QByteArray Base58::from(const QByteArray& data) const {
  CryptoPP::Integer int_data = 0;
  CryptoPP::Integer multiplier = 1;

  for (auto c : boost::adaptors::reverse(data)) {
    int_data += multiplier * current_alphabet.indexOf(c);
    multiplier *= 58;
  }

  int zeroes = 0;
  for (auto it = data.begin(); it != data.end() && *it == current_alphabet[0]; it++) zeroes += 1;

  return conv_integer(int_data).prepend(zeroes, '\0');
}

}  // namespace librevault::crypto
