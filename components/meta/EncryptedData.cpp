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
 */
#include "EncryptedData.h"
#include "AES_CBC.h"

namespace librevault {

EncryptedData EncryptedData::fromCiphertext(const QByteArray& ct, const QByteArray& iv) {
  if(iv.size() != 16)
    throw std::runtime_error("Invalid IV: " + iv.toHex());

  EncryptedData enc;
  enc.ct_ = ct;
  enc.iv_ = iv;
  enc.pt_.clear();
}

EncryptedData EncryptedData::fromPlaintext(const QByteArray& plaintext, const QByteArray& key, const QByteArray& iv) {
  if(iv.size() != 16)
    throw std::runtime_error("Invalid IV: " + iv.toHex());

  EncryptedData enc;
  enc.pt_ = plaintext;
  enc.iv_ = iv;
  enc.ct_ = encryptAesCbc(enc.pt_, key, enc.iv_);
}

const QByteArray& EncryptedData::plaintext(const QByteArray& key) const {
  if(pt_.isEmpty())
    pt_ = decryptAesCbc(ct_, key, iv_);
  return pt_;
}

const QByteArray& EncryptedData::ciphertext() const {
  return ct_;
}

const QByteArray& EncryptedData::iv() const {
  return iv_;
}

} /* namespace librevault */
