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
#include "Secret.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha3.h>

#include <QCryptographicHash>

#include "crypto/LuhnModN.h"

using CryptoPP::ASN1::secp256r1;

namespace librevault {

Secret::Secret() {
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;
  private_key.Initialize(rng, secp256r1());

  cached_private_key.resize(private_key_size);
  private_key.GetPrivateExponent().Encode((uchar*)cached_private_key.data(), private_key_size);

  secret_s += (char)Owner;
  secret_s += '1';
  secret_s += conv_bytearray(crypto::Base58().to(conv_bytearray(cached_private_key)));

  auto payload = secret_s.mid(2);
  secret_s += crypto::LuhnMod58(payload.begin(), payload.end());
}

Secret::Secret(Type type, const QByteArray& binary_part) {
  secret_s += type;
  secret_s += '1';
  secret_s += conv_bytearray(crypto::Base58().to(conv_bytearray(cached_private_key)));

  auto payload = secret_s.mid(2);
  secret_s += crypto::LuhnMod58(payload.begin(), payload.end());
}

Secret::Secret(const QString& str) : secret_s(str.toLatin1()) {
  try {
    auto base58_payload = get_encoded_payload();

    if (base58_payload.isEmpty()) throw format_error();
    if (crypto::LuhnMod58(base58_payload.begin(), base58_payload.end()) != get_check_char()) throw format_error();
  } catch (std::exception& e) {
    throw format_error();
  }

  // TODO: It would be good to check private/public key for validity and throw crypto_error() here
}

QByteArray Secret::get_encoded_payload() const {  // TODO: Caching
  return secret_s.mid(2, this->secret_s.size() - 3);
}

QByteArray Secret::get_payload() const {  // TODO: Caching
  return get_encoded_payload() | crypto::De<crypto::Base58>();
}

Secret Secret::derive(Type key_type) const {
  if (key_type == get_type()) return *this;

  switch (key_type) {
    case Owner:
    case ReadWrite:
      return Secret(key_type, get_Private_Key());
    case ReadOnly:
      return Secret(key_type, get_Public_Key() + get_Encryption_Key());
    case Download:
      return Secret(key_type, get_Encryption_Key());
    default:
      throw level_error();
  }
}

QByteArray Secret::get_Private_Key() const {
  if (!cached_private_key.isEmpty()) return cached_private_key;

  switch (get_type()) {
    case Owner:
    case ReadWrite:
      return cached_private_key = get_payload();
    default:
      throw level_error();
  }
}

QByteArray Secret::get_Encryption_Key() const {
  if (!cached_encryption_key.isEmpty()) return cached_encryption_key;

  switch (get_type()) {
    case Owner:
    case ReadWrite:
      return cached_encryption_key =
                 QCryptographicHash::hash(get_Private_Key(), QCryptographicHash::Algorithm::Sha3_256);
    case ReadOnly:
      return cached_encryption_key = get_payload().mid(public_key_size, encryption_key_size);
    default:
      throw level_error();
  }
}

QByteArray Secret::get_Public_Key() const {
  if (!cached_public_key.isEmpty()) return cached_public_key;

  switch (get_type()) {
    case Owner:
    case ReadWrite: {
      CryptoPP::AutoSeededRandomPool rng;
      CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;
      CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> public_key;
      private_key.Initialize(rng, secp256r1());
      private_key.SetPrivateExponent(conv_bytearray_to_integer(get_Private_Key()));
      private_key.MakePublicKey(public_key);

      public_key.AccessGroupParameters().SetPointCompression(true);
      cached_public_key.resize(public_key_size);
      public_key.GetGroupParameters().EncodeElement(true, public_key.GetPublicElement(),
                                                    (uchar*)cached_public_key.data());

      return cached_public_key;
    }
    case ReadOnly:
    case Download:
      return cached_public_key = get_payload().left(public_key_size);
    default:
      throw level_error();
  }
}

QByteArray Secret::get_Hash() const {
  if (!cached_hash.isEmpty()) return cached_hash;
  return cached_hash = QCryptographicHash::hash(get_Public_Key(), QCryptographicHash::Algorithm::Sha3_256);
}

std::ostream& operator<<(std::ostream& os, const Secret& k) { return os << k.string().toStdString(); }

}  // namespace librevault
