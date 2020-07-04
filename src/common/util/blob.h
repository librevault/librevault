#pragma once
#include <cryptopp/integer.h>

#include <QByteArray>

namespace librevault {

inline CryptoPP::Integer conv_integer(const QByteArray& ba) { return CryptoPP::Integer((uchar*)ba.data(), ba.size()); }

inline QByteArray conv_integer(const CryptoPP::Integer& i) {
  if (i == 0) return {};

  QByteArray decoded_number(i.MinEncodedSize(), '\0');
  i.Encode((uchar*)decoded_number.data(), decoded_number.size());
  return decoded_number;
}

}  // namespace librevault
