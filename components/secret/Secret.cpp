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
#include "LuhnModN.h"

#include <cryptopp/eccrypto.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <QCryptographicHash>

using CryptoPP::ASN1::secp256r1;

namespace librevault {

Secret::Secret() {
	CryptoPP::AutoSeededRandomPool rng;
	CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;
	private_key.Initialize(rng, secp256r1());

	cached_private_key.resize(private_key_size);
	private_key.GetPrivateExponent().Encode((uchar*)cached_private_key.data(), private_key_size);

	secret_s.append(1, (char)Owner);
	secret_s.append(1, '1');
	secret_s.append(toBase58(cached_private_key));
	secret_s.append(1, LuhnMod58(secret_s.data() + 2, secret_s.data() + secret_s.size()));
}

Secret::Secret(Type type, QByteArray binary_part) {
	secret_s.append(1, type);
	secret_s.append(1, '1');
	secret_s.append(toBase58(binary_part));
	secret_s.append(1, LuhnMod58(secret_s.data() + 2, secret_s.data() + secret_s.size()));
}

Secret::Secret(QString string_secret) : Secret(string_secret.toLatin1()) {}

Secret::Secret(QByteArray string_secret) : secret_s(string_secret) {
	auto base58_payload = getEncodedPayload();

	if(base58_payload.isEmpty()) throw format_error();
	if(LuhnMod58(base58_payload.begin(), base58_payload.end()) != getCheckChar()) throw format_error();

	// TODO: It would be good to check private/public key for validity and throw crypto_error() here
}

QByteArray Secret::getEncodedPayload() const {  // TODO: Caching
	return secret_s.mid(2, secret_s.size()-3);
}

QByteArray Secret::getPayload() const {	// TODO: Caching
	return fromBase58(getEncodedPayload());
}

Secret Secret::derive(Type key_type) const {
	if(key_type == getType()) return *this;

	switch(key_type){
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
	if(!cached_private_key.isEmpty()) return cached_private_key;

	switch(getType()){
	case Owner:
	case ReadWrite: {
		return cached_private_key = getPayload();
	}
	default:
		throw level_error();
	}
}

QByteArray Secret::getEncryptionKey() const {
	if(!cached_encryption_key.isEmpty()) return cached_encryption_key;

	switch(getType()){
	case Owner:
	case ReadWrite: {
		QCryptographicHash hash(QCryptographicHash::Sha3_256);
		hash.addData(getPrivateKey());

		return cached_encryption_key = hash.result();
	}
	case ReadOnly: {
		return cached_encryption_key = getPayload().mid(public_key_size, encryption_key_size);
	}
	default:
		throw level_error();
	}
}

QByteArray Secret::getPublicKey() const {
	if(!cached_public_key.isEmpty()) return cached_public_key;

	switch(getType()){
	case Owner:
	case ReadWrite: {
		CryptoPP::AutoSeededRandomPool rng;
		CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;
		CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> public_key;
		private_key.Initialize(rng, secp256r1());
		auto private_key_s = getPrivateKey();
		private_key.SetPrivateExponent(CryptoPP::Integer((uchar*)private_key_s.data(), private_key_s.size()));
		private_key.MakePublicKey(public_key);

		public_key.AccessGroupParameters().SetPointCompression(true);
		cached_public_key.resize(public_key_size);
		public_key.GetGroupParameters().EncodeElement(true, public_key.GetPublicElement(), (uchar*)cached_public_key.data());

		return cached_public_key;
	}
	case ReadOnly:
	case Download: {
		return cached_public_key = getPayload();
	}
	default:
		throw level_error();
	}
}

QByteArray Secret::getHash() const {
	if(!cached_hash.isEmpty()) return cached_hash;

	QCryptographicHash hash(QCryptographicHash::Sha3_256);
	hash.addData(getPublicKey());

	return cached_hash = hash.result();
}

std::ostream& operator<<(std::ostream& os, const Secret& k){
	return os << ((QString)k).toStdString();
}

} /* namespace librevault */
