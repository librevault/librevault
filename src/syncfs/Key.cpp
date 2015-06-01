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
#include <cryptopp/osrng.h>
#include <cryptopp/integer.h>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>

using CryptoPP::ASN1::secp256r1;

namespace librevault {
namespace syncfs {

Key::Key() {
	CryptoPP::AutoSeededRandomPool rng;
	CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;
	private_key.Initialize(rng, secp256r1());

	cached_private_key.resize(private_key_size);
	private_key.GetPrivateExponent().Encode(cached_private_key.data(), private_key_size);

	secret_s.append(1, (char)Owner);
	secret_s.append(crypto::Base58().to(cached_private_key));
	secret_s.append(1, crypto::LuhnMod58(&secret_s[1], &*secret_s.end()));
}

Key::Key(Type type, const std::vector<uint8_t>& binary_part) {
	secret_s.append(1, type);
	secret_s.append(crypto::Base58().to(binary_part));
	secret_s.append(1, crypto::LuhnMod58(&secret_s[1], &*secret_s.end()));
}

Key::Key(std::string string_secret) : secret_s(std::move(string_secret)) {
	auto base58_payload = secret_s.substr(1, this->secret_s.size()-2);
	if(crypto::LuhnMod58(base58_payload.begin(), base58_payload.end()) != getCheckChar()) throw format_error();

	// It would be good to check private/public key for validity and throw crypto_error() here
}

Key::~Key() {}

std::vector<uint8_t> Key::get_payload() const {
	return crypto::Base58().from(secret_s.substr(1, this->secret_s.size()-2));
}

Key Key::derive(Type key_type){
	if(key_type == getType()) return *this;

	switch(key_type){
	case Owner:
	case ReadWrite:
		return Key(key_type, get_Private_Key());
	case ReadOnly: {
		std::vector<uint8_t> new_payload(public_key_size+encryption_key_size);

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

std::vector<uint8_t>& Key::get_Private_Key() const {
	if(!cached_private_key.empty()) return cached_private_key;

	switch(getType()){
	case Owner:
	case ReadWrite: {
		cached_private_key = get_payload();
		return cached_private_key;
	}
	default:
		throw level_error();
	}
}

std::vector<uint8_t>& Key::get_Encryption_Key() const {
	if(!cached_encryption_key.empty()) return cached_encryption_key;

	switch(getType()){
	case Owner:
	case ReadWrite: {
		cached_encryption_key.resize(encryption_key_size);
		CryptoPP::SHA3_256().CalculateDigest(cached_encryption_key.data(), get_Private_Key().data(), get_Private_Key().size());
		return cached_encryption_key;
	}
	case ReadOnly: {
		auto payload = get_payload();
		cached_encryption_key.assign(&payload[public_key_size], &payload[public_key_size]+encryption_key_size);
		return cached_encryption_key;
	}
	default:
		throw level_error();
	}
}

std::vector<uint8_t>& Key::get_Public_Key() const {
	if(!cached_public_key.empty()) return cached_public_key;

	switch(getType()){
	case Owner:
	case ReadWrite: {
		CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;
		CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> public_key;
		private_key.SetPrivateExponent(CryptoPP::Integer(get_Private_Key().data(), get_Private_Key().size()));
		private_key.MakePublicKey(public_key);

		public_key.AccessGroupParameters().SetPointCompression(true);
		cached_public_key.resize(public_key_size);
		public_key.GetGroupParameters().EncodeElement(true, public_key.GetPublicElement(), cached_public_key.data());

		return cached_public_key;
	}
	case ReadOnly:
	case Download: {
		std::vector<uint8_t> payload = get_payload();
		cached_public_key.assign(&payload[0], &payload[0]+public_key_size);
		return cached_public_key;
	}
	default:
		throw level_error();
	}
}

} /* namespace syncfs */
} /* namespace librevault */
