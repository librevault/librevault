/* Copyright (C) 2015 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "AES_CBC.h"
#include "util/rand.h"
#include <cryptopp/aes.h>
#include <cryptopp/ccm.h>
#include <cryptopp/filters.h>

namespace librevault {

QByteArray generateRandomIV() { return getRandomArray(16); }

QByteArray encryptAesCbc(
    const QByteArray& src, const QByteArray& key, QByteArray iv, bool padding) {
  iv = iv.leftJustified(16, 0, true);

  CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption filter(
      (const uchar*)key.data(), key.size(), (const uchar*)iv.constData());

  std::string buffer;
  CryptoPP::StringSource((const uchar*)src.data(), src.size(), true,
      new CryptoPP::StreamTransformationFilter(filter, new CryptoPP::StringSink(buffer),
          padding ? CryptoPP::StreamTransformationFilter::PKCS_PADDING
                  : CryptoPP::StreamTransformationFilter::NO_PADDING));

  return QByteArray::fromStdString(buffer);
}

QByteArray decryptAesCbc(
    const QByteArray& src, const QByteArray& key, QByteArray iv, bool padding) {
  iv = iv.leftJustified(16, 0, true);

  CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption filter(
      (const uchar*)key.data(), key.size(), (const uchar*)iv.constData());

  std::string buffer;
  CryptoPP::StringSource((const uchar*)src.data(), src.size(), true,
      new CryptoPP::StreamTransformationFilter(filter, new CryptoPP::StringSink(buffer),
          padding ? CryptoPP::StreamTransformationFilter::PKCS_PADDING
                  : CryptoPP::StreamTransformationFilter::NO_PADDING));

  return QByteArray::fromStdString(buffer);
}

}  // namespace librevault
