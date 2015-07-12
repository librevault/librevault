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
#pragma once
#include "types.h"

//#include <botan/ecdsa.h>

#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>
#include <cryptopp/aes.h>
#include <cryptopp/ccm.h>
#include <cryptopp/filters.h>
#include <cryptopp/cryptlib.h>

//#include <openssl/pem.h>
#include <openssl/x509.h>

#include <cstdio>
#include <iostream>

namespace librevault {

class NodeKey {
	using private_key_t = CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP>;

	private_key_t private_key;

	EVP_PKEY* openssl_pkey;

	fs::path key, cert;
public:
	NodeKey(fs::path key, fs::path cert);
	virtual ~NodeKey();

	CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP>& gen_private_key();
	void write_key();

	void gen_certificate();

	void write_cert();

	const fs::path& get_key_path() const {return key;}
	const fs::path& get_cert_path() const {return cert;}

	const CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP>& get_private_key() const {return private_key;}
};

} /* namespace librevault */
