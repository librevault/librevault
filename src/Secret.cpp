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
#include "include/librevault/Secret.h"
#include "include/librevault/crypto/LuhnModN.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/oids.h>
#include <cryptopp/sha3.h>
#include <cryptopp/osrng.h>

using CryptoPP::ASN1::secp256r1;

namespace librevault {

Secret::Secret() {
	CryptoPP::AutoSeededRandomPool rng;
	CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;
	private_key.Initialize(rng, secp256r1());

	cached_private_key.resize(private_key_size);
	private_key.GetPrivateExponent().Encode(cached_private_key.data(), private_key_size);

	secret_s.append(1, (char)Owner);
	secret_s.append(1, '1');
	secret_s.append(crypto::Base58().to_string(cached_private_key));
	secret_s.append(1, crypto::LuhnMod58(&secret_s[2], &*secret_s.end()));
}

Secret::Secret(Type type, const std::vector<uint8_t>& binary_part) {
	secret_s.append(1, type);
	secret_s.append(1, '1');
	secret_s.append(crypto::Base58().to_string(binary_part));
	secret_s.append(1, crypto::LuhnMod58(&secret_s[2], &*secret_s.end()));
}

Secret::Secret(std::string string_secret) : secret_s(std::move(string_secret)) {
	try {
		auto base58_payload = get_encoded_payload();

		if(base58_payload.empty()) throw format_error();
		if(crypto::LuhnMod58(base58_payload.begin(), base58_payload.end()) != get_check_char()) throw format_error();
	}catch(std::exception& e){
		throw format_error();
	}

	// TODO: It would be good to check private/public key for validity and throw crypto_error() here
}

std::string Secret::get_encoded_payload() const {  // TODO: Caching
	return secret_s.substr(2, this->secret_s.size()-3);
}

std::vector<uint8_t> Secret::get_payload() const {	// TODO: Caching
	return get_encoded_payload() | crypto::De<crypto::Base58>();
}

Secret Secret::derive(Type key_type) const {
	if(key_type == get_type()) return *this;

	switch(key_type){
	case Owner:
	case ReadWrite:
		return Secret(key_type, get_Private_Key());
	case ReadOnly: {
		std::vector<uint8_t> new_payload(public_key_size+encryption_key_size);

		auto& public_key_s = get_Public_Key();
		std::copy(public_key_s.begin(), public_key_s.end(), new_payload.begin());

		auto& encryption_key_s = get_Encryption_Key();
		std::copy(encryption_key_s.begin(), encryption_key_s.end(), &new_payload[public_key_size]);

		return Secret(key_type, new_payload);
	}
	case Download:
		return Secret(key_type, get_Encryption_Key());
	default:
		throw level_error();
	}
}

const std::vector<uint8_t>& Secret::get_Private_Key() const {
	if(!cached_private_key.empty()) return cached_private_key;

	switch(get_type()){
	case Owner:
	case ReadWrite: {
		cached_private_key = get_payload();
		return cached_private_key;
	}
	default:
		throw level_error();
	}
}

const std::vector<uint8_t>& Secret::get_Encryption_Key() const {
	if(!cached_encryption_key.empty()) return cached_encryption_key;

	switch(get_type()){
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

const std::vector<uint8_t>& Secret::get_Public_Key() const {
	if(!cached_public_key.empty()) return cached_public_key;

	switch(get_type()){
	case Owner:
	case ReadWrite: {
		CryptoPP::AutoSeededRandomPool rng;
		CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;
		CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> public_key;
		private_key.Initialize(rng, secp256r1());
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

const std::vector<uint8_t>& Secret::get_Hash() const {
	if(!cached_hash.empty()) return cached_hash;

	cached_hash.resize(hash_size);
	CryptoPP::SHA3_256().CalculateDigest(cached_hash.data(), get_Public_Key().data(), get_Public_Key().size());
	return cached_hash;
}

std::ostream& operator<<(std::ostream& os, const Secret& k){
	return os << k.string();
}

} /* namespace librevault */
