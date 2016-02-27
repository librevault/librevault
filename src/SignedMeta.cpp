/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include <include/librevault/SignedMeta.h>
#include <cryptopp/osrng.h>
#include <cryptopp/ecp.h>
#include <cryptopp/sha3.h>
#include <cryptopp/oids.h>
#include <cryptopp/eccrypto.h>

namespace librevault {

SignedMeta::SignedMeta(Meta meta, const Secret& secret) {
	meta_ = std::make_shared<Meta>(std::move(meta));
	raw_meta_ = std::make_shared<blob>(meta.serialize());

	CryptoPP::AutoSeededRandomPool rng;
	CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Signer signer;
	signer.AccessKey().Initialize(CryptoPP::ASN1::secp256r1(), CryptoPP::Integer(secret.get_Private_Key().data(), secret.get_Private_Key().size()));

	signature_ = std::make_shared<blob>(signer.SignatureLength());
	signer.SignMessage(rng, raw_meta_->data(), raw_meta_->size(), signature_->data());
}

SignedMeta::SignedMeta(blob raw_meta, blob signature, const Secret& secret, bool check_signature) :
	raw_meta_(std::make_shared<blob>(std::move(raw_meta))),
	signature_(std::make_shared<blob>(std::move(signature))) {

	if(check_signature) {
		try {
			CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA3_256>::Verifier verifier;
			verifier.AccessKey().AccessGroupParameters() = CryptoPP::ASN1::secp256r1();
			verifier.AccessKey().AccessGroupParameters().SetPointCompression(true);
			verifier.AccessKey().AccessGroupParameters().DecodeElement(secret.get_Public_Key().data(), true);
			if(! verifier.VerifyMessage(raw_meta_->data(), raw_meta_->size(), signature_->data(), signature_->size()))
				throw signature_error();
		}catch(CryptoPP::Exception& e){
			throw signature_error();
		}
	}

	meta_ = std::make_shared<Meta>(*raw_meta_);
}

} /* namespace librevault */
