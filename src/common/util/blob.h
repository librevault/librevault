#pragma once
#include <cryptopp/integer.h>

#include <QByteArray>

namespace librevault {

inline CryptoPP::Integer conv_bytearray_to_integer(const QByteArray& ba) {
  return CryptoPP::Integer((uchar*)ba.data(), ba.size());
}

}  // namespace librevault
