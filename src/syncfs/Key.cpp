/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Key.h"

#include "../../contrib/crypto/LuhnModN.h"
#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/oids.h>
#include <cryptopp/sha3.h>

using CryptoPP::ASN1::secp256r1;

namespace librevault {
namespace syncfs {

Key::Key() {
}

Key::Key(Type type, std::vector<uint8_t> binary_part) : payload(std::move(binary_part)) {
	secret_s.append(1, type);
	secret_s.append(crypto::Base58().to(binary_part));
	secret_s.append(1, crypto::LuhnMod58(&secret_s[1], &*secret_s.end()));
}

Key::Key(std::string secret_s) : secret_s(std::move(secret_s)) {
	const std::string payload_s = secret_s.substr(1, this->secret_s.size()-2);

	if(crypto::LuhnMod58(payload_s.begin(), payload_s.end()) != getCheckChar()) throw format_error();
	payload = crypto::Base58().from(payload_s);

	private_key.AccessGroupParameters().SetPointCompression(true);
	public_key.AccessGroupParameters().SetPointCompression(true);
	encryption_key.resize(CryptoPP::SHA3_256().DigestSize());

	try {
		switch(getType()){
		case Owner:
		case ReadWrite:
			private_key.SetPrivateExponent(CryptoPP::Integer(payload.data(), payload.size()));
			private_key.MakePublicKey(public_key);
			CryptoPP::SHA3_256().CalculateDigest(encryption_key.data(), payload.data(), payload.size());
			break;
		case Download:
			public_key.SetPublicElement(public_key.AccessGroupParameters().DecodeElement(payload.data(), true));
			// no break
		case ReadOnly:
			encryption_key.assign(&payload[33], &*payload.end());
			break;
		default:
			throw format_error();
		}
	}catch(CryptoPP::DL_BadElement& e){
		throw crypto_error();
	}
}

Key::~Key() {}

Key Key::derive(Type key_type){
	if(key_type == getType()) return *this;

	switch(key_type){
	case Owner:
	case ReadWrite:
		return Key(key_type, get_Private_Key());
	case ReadOnly: {
		std::vector<uint8_t> new_payload(65);

		auto public_key_s = get_Public_Key();
		std::copy(public_key_s.begin(), public_key_s.end(), new_payload.begin());

		auto encryption_key_s = get_Encryption_Key();
		std::copy(encryption_key_s.begin(), encryption_key_s.end(), &new_payload[public_key_size]);

		return Key(key_type, new_payload);
	}
	case Download:
		return Key(key_type, get_Encryption_Key());
	default:
		throw level_error();
	}
}

std::vector<uint8_t> Key::get_Private_Key() const {
	switch(getType()){
	case Owner:
	case ReadWrite: {
		std::vector<uint8_t> encoded_private_key(32);
		private_key.GetPrivateExponent().Encode(encoded_private_key.data(), encoded_private_key.size(), CryptoPP::Integer::UNSIGNED);
		return encoded_private_key;
	}
	default:
		throw level_error();
	}
}

std::vector<uint8_t> Key::get_Public_Key() const {
	switch(getType()){
	case Owner:
	case ReadWrite:
	case ReadOnly: {
		std::vector<uint8_t> encoded_public_key(public_key.GetGroupParameters().GetEncodedElementSize(true));
		public_key.GetGroupParameters().EncodeElement(true, public_key.GetPublicElement(), encoded_public_key.data());
		return encoded_public_key;
	}
	default:
		throw level_error();
	}
}

std::vector<uint8_t> Key::get_Encryption_Key() const {
	return encryption_key;
}

} /* namespace syncfs */
} /* namespace librevault */
