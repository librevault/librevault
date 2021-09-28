#include "Secret.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>

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
  secret_s += cached_private_key | crypto::Base58();

  auto payload = secret_s.mid(2);
  secret_s += crypto::LuhnMod58(payload);
}

Secret::Secret(Type type, const QByteArray& binary_part) {
  secret_s += type;
  secret_s += '1';
  secret_s += binary_part | crypto::Base58();

  auto payload = secret_s.mid(2);
  secret_s += crypto::LuhnMod58(payload);
}

Secret::Secret(const QString& str) : secret_s(str.toLatin1()) {
  try {
    auto base58_payload = getEncodedPayload();

    if (base58_payload.isEmpty()) throw format_error();
    if (crypto::LuhnMod58(base58_payload) != get_check_char()) throw format_error();
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
      return {key_type, get_Private_Key()};
    case ReadOnly:
      return {key_type, get_Public_Key() + get_Encryption_Key()};
    case Download:
      return {key_type, get_Encryption_Key()};
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
      private_key.SetPrivateExponent(conv_integer(get_Private_Key()));
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

std::ostream& operator<<(std::ostream& os, const Secret& k) { return os << k.string().toStdString(); }

}  // namespace librevault
