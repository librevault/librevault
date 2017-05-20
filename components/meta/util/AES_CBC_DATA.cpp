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
#include "AES_CBC_DATA.h"
#include "AES_CBC.h"
#include "../Meta.h"
#include "blob.h"
#include <cryptopp/cryptlib.h>

namespace librevault {

bool AES_CBC_DATA::check(const Secret& secret) {
	if(!check()) return false;
	try {
		decryptAesCbc(ct, secret.getEncryptionKey(), iv);
	}catch(const CryptoPP::Exception& e){
		return false;
	}
	return true;
}

std::vector<uint8_t> AES_CBC_DATA::get_plain(const Secret& secret) const {
	try {
		return conv_bytearray(decryptAesCbc(ct, secret.getEncryptionKey(), iv));
	}catch(const CryptoPP::Exception& e){
		throw Meta::parse_error("Parse error: Decryption failed");
	}
}

void AES_CBC_DATA::set_plain(const std::vector<uint8_t>& pt, const Secret& secret) {
	iv = generateRandomIV();
	ct = encryptAesCbc(ct, secret.getEncryptionKey(), conv_bytearray(pt));
}

} /* namespace librevault */
