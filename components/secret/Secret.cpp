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
#include "Secret.h"
#include "SecretPrivate.h"
#include "LuhnModN.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <QCryptographicHash>

using CryptoPP::ASN1::secp256r1;

namespace librevault {

namespace {
constexpr size_t private_key_size = 32;
constexpr size_t encryption_key_size = 32;
constexpr size_t public_key_size = 33;

constexpr size_t hash_size = 32;

char computeCheckChar(const QByteArray& data) {
  return LuhnMod58(data.begin(), data.end());
}

QByteArray generatePrivateKey() {
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key_crypto;
  private_key_crypto.Initialize(rng, secp256r1());

  QByteArray private_key(private_key_size, 0);
  private_key_crypto.GetPrivateExponent().Encode((uchar*)private_key.data(), private_key_size);
  return private_key;
}

QByteArray derivePublicKey(const QByteArray& private_key) {
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key_crypto;
  CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> public_key_crypto;
  private_key_crypto.Initialize(rng, secp256r1());
  private_key_crypto.SetPrivateExponent(CryptoPP::Integer((uchar*)private_key.data(), private_key.size()));
  private_key_crypto.MakePublicKey(public_key_crypto);

  public_key_crypto.AccessGroupParameters().SetPointCompression(true);
  QByteArray public_key(public_key_size, 0);
  public_key_crypto.GetGroupParameters().EncodeElement(true, public_key_crypto.GetPublicElement(), (uchar*)public_key.data());
  return public_key;
}
}

Secret::Secret() : Secret(Owner, generatePrivateKey()) {}

Secret::Secret(Type type, QByteArray binary_part) {
  d = new SecretPrivate();

  d->secret_s += type;
  d->secret_s += '1';
  d->secret_s += toBase58(binary_part);
  d->secret_s += computeCheckChar(d->secret_s.mid(2));
}

Secret::Secret(const QByteArray& string_secret) {
  d = new SecretPrivate();

  d->secret_s = string_secret;
  auto base58_payload = getEncodedPayload();

  if (base58_payload.isEmpty()) throw format_error();
  if (computeCheckChar(base58_payload) != getCheckChar()) throw format_error();

  // TODO: It would be good to check private/public key for validity and throw crypto_error() here
}

QByteArray Secret::getEncodedPayload() const {  // TODO: Caching
  return d->secret_s.mid(2, d->secret_s.size() - 3);
}

QByteArray Secret::getPayload() const {  // TODO: Caching
  return fromBase58(getEncodedPayload());
}

Secret Secret::derive(Type key_type) const {
  if (key_type == getType()) return *this;

  switch (key_type) {
    case Owner:
    case ReadWrite:
      return Secret(key_type, getPrivateKey());
    case ReadOnly:
      return Secret(key_type, getPublicKey() + getEncryptionKey());
    case Download:
      return Secret(key_type, getPublicKey());
    default:
      throw level_error();
  }
}

QByteArray Secret::getPrivateKey() const {
  if (!d->cached_private_key.isEmpty()) return d->cached_private_key;

  switch (getType()) {
    case Owner:
    case ReadWrite: {
      return d->cached_private_key = getPayload();
    }
    default:
      throw level_error();
  }
}

QByteArray Secret::getEncryptionKey() const {
  if (!d->cached_encryption_key.isEmpty()) return d->cached_encryption_key;

  switch (getType()) {
    case Owner:
    case ReadWrite: {
      QCryptographicHash hash(QCryptographicHash::Sha3_256);
      hash.addData(getPrivateKey());

      return d->cached_encryption_key = hash.result();
    }
    case ReadOnly: {
      return d->cached_encryption_key = getPayload().mid(public_key_size, encryption_key_size);
    }
    default:
      throw level_error();
  }
}

QByteArray Secret::getPublicKey() const {
  if (!d->cached_public_key.isEmpty()) return d->cached_public_key;

  switch (getType()) {
    case Owner:
    case ReadWrite: {
      d->cached_public_key = derivePublicKey(getPrivateKey());
      return d->cached_public_key;
    }
    case ReadOnly:
    case Download: {
      return d->cached_public_key = getPayload();
    }
    default:
      throw level_error();
  }
}

QByteArray Secret::getHash() const {
  if (!d->cached_hash.isEmpty()) return d->cached_hash;

  QCryptographicHash hash(QCryptographicHash::Sha3_256);
  hash.addData(getPublicKey());

  return d->cached_hash = hash.result();
}

std::ostream& operator<<(std::ostream& os, const Secret& k) { return os << ((QString)k).toStdString(); }

} /* namespace librevault */
