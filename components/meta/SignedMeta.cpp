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
#include <cryptopp/osrng.h>
#include <cryptopp/ecp.h>
#include <cryptopp/sha3.h>
#include <cryptopp/oids.h>
#include <cryptopp/eccrypto.h>

namespace librevault {

SignedMeta::SignedMeta(Meta meta, const Secret& secret) {
	meta_ = std::make_shared<Meta>(std::move(meta));
	raw_meta_ = meta.serialize();

	CryptoPP::AutoSeededRandomPool rng;
	CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Signer signer;
	auto privkey_s = secret.getPrivateKey();
	signer.AccessKey().Initialize(CryptoPP::ASN1::secp256r1(), CryptoPP::Integer((uchar*)privkey_s.data(), privkey_s.size()));

	signature_ = QByteArray(signer.SignatureLength(), 0);
	signer.SignMessage(rng, (uchar*)raw_meta_.data(), raw_meta_.size(), (uchar*)signature_.data());
}

SignedMeta::SignedMeta(QByteArray raw_meta, QByteArray signature) : raw_meta_(raw_meta), signature_(signature) {
	meta_ = std::make_shared<Meta>(raw_meta_);
}

bool SignedMeta::isValid(const Secret& secret) const {
	try {
		CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Verifier verifier;
		CryptoPP::ECP::Point p;

		verifier.AccessKey().AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());
		verifier.AccessKey().AccessGroupParameters().SetPointCompression(true);
		auto pubkey_s = secret.getPublicKey();
		verifier.AccessKey().GetGroupParameters().GetCurve().DecodePoint(p, (uchar*)pubkey_s.data(), pubkey_s.size());
		verifier.AccessKey().SetPublicElement(p);
		if(! verifier.VerifyMessage((uchar*)raw_meta_.data(), raw_meta_.size(), (uchar*)signature_.data(), signature_.size()))
			return false;
	}catch(CryptoPP::Exception& e){
		return false;
	}

	return true;
}

} /* namespace librevault */
