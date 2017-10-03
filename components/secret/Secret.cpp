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
constexpr int private_key_size = 32;
constexpr int encryption_key_size = 32;
constexpr int public_key_size = 33;

constexpr int hash_size = 32;

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

QByteArray deriveEncryptionKey(const QByteArray& private_key) {
  QCryptographicHash hash(QCryptographicHash::Sha3_256);
  hash.addData(private_key);
  return hash.result();
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

QByteArray makeFolderHash(const QByteArray& public_key) {
  QCryptographicHash hash(QCryptographicHash::Sha3_256);
  hash.addData(public_key);
  return hash.result();
}
}

Secret::Secret() {}

Secret::Secret(const QByteArray& string_secret) {
  d = new SecretPrivate();
  if(string_secret.size() < 3) throw FormatMismatch();  // Level, Version, Checksum

  // Extract version
  d->version_ = string_secret[1];
  if(d->version_ != '1') throw UnknownSecretVersion();

  // Extract type
  d->level_ = string_secret[0];
  if(d->level_ < Owner || d->level_ > Download) throw UnknownSecretLevel();

  // Extract payload
  QByteArray base58_payload = string_secret.mid(2, string_secret.size()-3);
  if (base58_payload.isEmpty()) throw FormatMismatch();
  QByteArray payload = fromBase58(base58_payload);

  if(d->level_ == Owner || d->level_ == ReadWrite) {
    if(payload.size() < private_key_size) throw FormatMismatch();
    d->private_key_ = payload;
    d->encryption_key_ = deriveEncryptionKey(d->private_key_);
    d->public_key_ = derivePublicKey(d->private_key_);
  }else if(d->level_ == ReadOnly) {
    if(payload.size() < public_key_size + encryption_key_size) throw FormatMismatch();
    d->encryption_key_ = payload.mid(public_key_size, encryption_key_size);
    d->public_key_ = payload.mid(0, public_key_size);
  }else if(d->level_ == Download) {
    if(payload.size() < public_key_size) throw FormatMismatch();
    d->public_key_ = payload;
  }
  d->folderid_ = makeFolderHash(d->public_key_);

  // Extract Luhn checksum
  char checksum = *(string_secret.rbegin());
  if(computeCheckChar(base58_payload) != checksum) throw FormatMismatch();

  // TODO: It would be good to check private/public key for validity and throw CryptoError() here
}

Secret Secret::generate() {
  QByteArray secret_s;
  secret_s += 'A';
  secret_s += '1';
  QByteArray base58_payload = toBase58(generatePrivateKey());
  secret_s += base58_payload;
  secret_s += computeCheckChar(base58_payload);

  return Secret(secret_s);
}

QString Secret::toString() const {
  Q_ASSERT(d);

  QString s;
  s.reserve(128);
  s += d->level_;
  s += d->version_;
  QByteArray base58_payload = toBase58(makeBinaryPayload());
  s += base58_payload;
  s += computeCheckChar(base58_payload);
  s.squeeze();

  return s;
}

Secret Secret::derive(Level key_type) const {
  Q_ASSERT(d);
  if(currentLevelLessThan(key_type)) throw LevelError();

  Secret s(*this);
  s.d->level_ = key_type;
  return s;
}

const QByteArray& Secret::privateKey() const {
  Q_ASSERT(d);
  if(currentLevelLessThan(ReadWrite)) throw LevelError();
  return d->private_key_;
}

const QByteArray& Secret::encryptionKey() const {
  Q_ASSERT(d);
  if(currentLevelLessThan(ReadOnly)) throw LevelError();
  return d->encryption_key_;
}

const QByteArray& Secret::publicKey() const {
  Q_ASSERT(d);
  if(currentLevelLessThan(Download)) throw LevelError();
  return d->public_key_;
}

const QByteArray& Secret::folderid() const {
  Q_ASSERT(d);
  return d->folderid_;
}

QByteArray Secret::makeBinaryPayload() const {
  Q_ASSERT(d);
  switch(d->level_) {
    case Secret::Owner:
    case Secret::ReadWrite:
      return d->private_key_;
    case Secret::ReadOnly:
      return d->public_key_ + d->encryption_key_;
    case Secret::Download:
      return d->public_key_;
    default:
      throw UnknownSecretLevel();
  }
}

bool Secret::currentLevelLessThan(Level lvl) const {
  Q_ASSERT(d);
  if(lvl < Owner || lvl > Download)
    throw UnknownSecretLevel();
  if( (lvl == Owner || lvl == ReadWrite) && (d->level_ == Owner || d->level_ == ReadWrite) )
    return false;
  if(lvl >= d->level_)
    return false;

  return true;
}

std::ostream& operator<<(std::ostream& os, const Secret& k) { return os << k.toString().toStdString(); }

} /* namespace librevault */
