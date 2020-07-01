#include "Base32.h"

#include <cryptopp/base32.h>

namespace librevault::crypto {

QByteArray Base32::to(const QByteArray& data) const {
  std::string transformed;
  CryptoPP::StringSource((const uchar*)data.data(), data.size(), true,
                         new CryptoPP::Base32Encoder(new CryptoPP::StringSink(transformed)));

  return QByteArray::fromStdString(transformed);
}

QByteArray Base32::from(const QByteArray& data) const {
  std::string transformed;
  CryptoPP::StringSource((const uchar*)data.data(), data.size(), true,
                         new CryptoPP::Base32Decoder(new CryptoPP::StringSink(transformed)));

  return QByteArray::fromStdString(transformed);
}

}  // namespace librevault::crypto
