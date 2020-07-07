#include "SignedMeta.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha3.h>

#include <utility>

#include "util/blob.h"

namespace librevault {

SignedMeta::SignedMeta(Meta meta, const Secret& secret) : meta_(meta) {
  raw_meta_ = meta_->serialize();

  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Signer signer;
  signer.AccessKey().Initialize(CryptoPP::ASN1::secp256r1(), conv_integer(secret.get_Private_Key()));

  signature_ = QByteArray(signer.SignatureLength(), 0);
  signer.SignMessage(rng, (const uchar*)raw_meta_.data(), raw_meta_.size(), (uchar*)signature_.data());
}

SignedMeta::SignedMeta(const QByteArray& raw_meta, QByteArray signature, const Secret& secret, bool check_signature)
    : meta_(raw_meta), raw_meta_(raw_meta), signature_(std::move(signature)) {
  if (!check_signature) return;

  try {
    auto public_key = secret.get_Public_Key();

    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Verifier verifier;
    CryptoPP::ECP::Point p;

    verifier.AccessKey().AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());
    verifier.AccessKey().AccessGroupParameters().SetPointCompression(true);
    verifier.AccessKey().GetGroupParameters().GetCurve().DecodePoint(p, (uchar*)public_key.data(), public_key.size());
    verifier.AccessKey().SetPublicElement(p);
    if (!verifier.VerifyMessage((const uchar*)raw_meta_.data(), raw_meta_.size(), (const uchar*)signature_.data(),
                                signature_.size()))
      throw SignatureError();
  } catch (CryptoPP::Exception& e) {
    throw SignatureError();
  }
}

}  // namespace librevault
