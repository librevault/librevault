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
#include <librevaultrs.hpp>

#include <crypto/Base58.h>

using CryptoPP::ASN1::secp256r1;

namespace librevault {

char LuhnMod58(const QByteArray& message) {
  return calc_luhnmod58({reinterpret_cast<const uint8_t*>(message.data()), static_cast<uintptr_t>(message.size())});
}

Secret::Secret() {
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;
  private_key.Initialize(rng, secp256r1());

  cached_private_key.resize(private_key_size);
  private_key.GetPrivateExponent().Encode((uchar*)cached_private_key.data(), private_key_size);

  secret_s += (char)Owner;
  secret_s += '1';
  secret_s += cached_private_key | crypto::Base58();

  auto payload = secret_s.mid(2);
  secret_s += LuhnMod58(payload);
}

Secret::Secret(Type type, const QByteArray& binary_part) {
  secret_s += type;
  secret_s += '1';
  secret_s += binary_part | crypto::Base58();

  auto payload = secret_s.mid(2);
  secret_s += LuhnMod58(payload);
}

Secret::Secret(const QString& str) : secret_s(str.toLatin1()) {
  try {
    auto base58_payload = getEncodedPayload();

    if (base58_payload.isEmpty()) throw format_error();
    if (LuhnMod58(base58_payload) != get_check_char()) throw format_error();
  } catch (std::exception& e) {
    throw format_error();
  }

  // TODO: It would be good to check private/public key for validity and throw crypto_error() here
}

QByteArray Secret::getEncodedPayload() const {  // TODO: Caching
  return secret_s.mid(2, this->secret_s.size() - 3);
}

QByteArray Secret::getPayload() const {  // TODO: Caching
  return getEncodedPayload() | crypto::De<crypto::Base58>();
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
      return cached_private_key = getPayload();
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
      return cached_encryption_key = getPayload().mid(public_key_size, encryption_key_size);
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
      return cached_public_key = getPayload().left(public_key_size);
    default:
      throw level_error();
  }
}

QByteArray Secret::get_Hash() const {
  if (!cached_hash.isEmpty()) return cached_hash;
  return cached_hash = QCryptographicHash::hash(get_Public_Key(), QCryptographicHash::Algorithm::Sha3_256);
}

bool Secret::verify(const QByteArray& message, const QByteArray& signature) const {
  try {
    auto public_key = get_Public_Key();

    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Verifier verifier;
    CryptoPP::ECP::Point p;

    verifier.AccessKey().AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());
    verifier.AccessKey().AccessGroupParameters().SetPointCompression(true);
    verifier.AccessKey().GetGroupParameters().GetCurve().DecodePoint(p, (uchar*)public_key.data(), public_key.size());
    verifier.AccessKey().SetPublicElement(p);
    if (!verifier.VerifyMessage((const uchar*)message.data(), message.size(), (const uchar*)signature.data(),
                                signature.size()))
      return false;
  } catch (CryptoPP::Exception& e) {
    return false;
  }
  return true;
}

QByteArray Secret::sign(const QByteArray& message) const {
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Signer signer;
  signer.AccessKey().Initialize(CryptoPP::ASN1::secp256r1(), conv_bytearray_to_integer(get_Private_Key()));

  auto signature = QByteArray(signer.SignatureLength(), 0);
  signer.SignMessage(rng, (const uchar*)message.data(), message.size(), (uchar*)signature.data());
  return signature;
}

std::ostream& operator<<(std::ostream& os, const Secret& k) { return os << k.string().toStdString(); }

}  // namespace librevault
