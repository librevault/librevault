#include "AES_CBC.h"

#include <cryptopp/aes.h>
#include <cryptopp/ccm.h>
#include <cryptopp/filters.h>

namespace librevault::crypto {

AES_CBC::AES_CBC(const QByteArray& key, const QByteArray& iv, bool padding) : key(key), iv(iv), padding(padding) {}

QByteArray AES_CBC::encrypt(const QByteArray& plaintext) const {
  CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption filter((const uchar*)key.data(), key.size(), (const uchar*)iv.data());

  std::string ciphertext;
  CryptoPP::StringSource(
      (const uchar*)plaintext.data(), plaintext.size(), true,
      new CryptoPP::StreamTransformationFilter(filter, new CryptoPP::StringSink(ciphertext),
                                               padding ? CryptoPP::StreamTransformationFilter::PKCS_PADDING
                                                       : CryptoPP::StreamTransformationFilter::NO_PADDING));

  return QByteArray::fromStdString(ciphertext);
}

QByteArray AES_CBC::decrypt(const QByteArray& ciphertext) const {
  CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption filter((const uchar*)key.data(), key.size(), (const uchar*)iv.data());

  std::string plaintext;
  CryptoPP::StringSource(
      (const uchar*)ciphertext.data(), ciphertext.size(), true,
      new CryptoPP::StreamTransformationFilter(filter, new CryptoPP::StringSink(plaintext),
                                               padding ? CryptoPP::StreamTransformationFilter::PKCS_PADDING
                                                       : CryptoPP::StreamTransformationFilter::NO_PADDING));

  return QByteArray::fromStdString(plaintext);
}

}  // namespace librevault::crypto
