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
#include "SignedMeta.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha3.h>

#include "util/blob.h"

namespace librevault {

SignedMeta::SignedMeta(Meta meta, const Secret& secret) {
  meta_ = std::make_shared<Meta>(std::move(meta));
  raw_meta_ = meta.serialize();

  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Signer signer;
  signer.AccessKey().Initialize(CryptoPP::ASN1::secp256r1(), conv_bytearray_to_integer(secret.get_Private_Key()));

  signature_ = QByteArray(signer.SignatureLength(), 0);
  signer.SignMessage(rng, (const uchar*)raw_meta_.data(), raw_meta_.size(), (uchar*)signature_.data());
}

SignedMeta::SignedMeta(std::vector<uint8_t> raw_meta, std::vector<uint8_t> signature, const Secret& secret,
                       bool check_signature)
    : raw_meta_(conv_bytearray(raw_meta)), signature_(conv_bytearray(signature)) {
  if (check_signature) {
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
        throw signature_error();
    } catch (CryptoPP::Exception& e) {
      throw signature_error();
    }
  }

  meta_ = std::make_shared<Meta>(raw_meta_);
}

}  // namespace librevault
