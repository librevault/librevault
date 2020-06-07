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
#include "AES_CBC.h"

#include <cryptopp/aes.h>
#include <cryptopp/ccm.h>
#include <cryptopp/filters.h>

namespace librevault {
namespace crypto {

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

}  // namespace crypto
}  // namespace librevault
