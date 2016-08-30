/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
#pragma once
#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <openssl/x509.h>
#include "util/blob.h"
#include "util/Loggable.h"

namespace librevault {

class NodeKey : protected Loggable {
public:
	using private_key_t = CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP>;

	NodeKey();
	virtual ~NodeKey();

	const private_key_t& private_key() const {return private_key_;}
	const blob& public_key() const {return public_key_;}

private:
	private_key_t private_key_;
	blob public_key_;
	EVP_PKEY* openssl_pkey_;	// Yes, we have both Crypto++ and OpenSSL-format private keys, because we have to use both libraries.

	X509* x509_;	// X509 structure pointer from OpenSSL

	private_key_t& gen_private_key();
	void write_key();

	void gen_certificate();
};

} /* namespace librevault */
