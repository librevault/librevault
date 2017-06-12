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
#include "Base58.h"
#include <cryptopp/integer.h>

namespace librevault {

QByteArray toBase58(const QByteArray& src) {
	CryptoPP::Integer big_data((const uchar*)src.data(), src.size());

	QByteArray result;
	result.reserve(src.size()*138/100 + 1);

	CryptoPP::word mod;
	CryptoPP::Integer div;
	while(big_data > 0){
		CryptoPP::Integer::Divide(mod, div, big_data, 58);
		result += base58_alphabet.at(mod);
		big_data = div;
	}

	for(const char* orig_str = src.data(); orig_str < src.data()+src.size() && *orig_str == 0; orig_str++){
		result += base58_alphabet[0];
	}

	std::reverse(result.begin(), result.end());
	return result;
}

QByteArray fromBase58(const QByteArray& src) {
	CryptoPP::Integer big_data = 0;
	CryptoPP::Integer multi = 1;

	for(int i = src.size()-1; i >= 0; i--){
		big_data += multi * base58_alphabet.indexOf(src[i]);
		multi *= 58;
	}

	int leading_zeros = 0;
	for(const char* orig_str = src.data(); orig_str < src.data()+src.size() && *orig_str == base58_alphabet[0]; orig_str++){
		leading_zeros++;
	}

	QByteArray decoded(leading_zeros + big_data.MinEncodedSize(), 0);
	big_data.Encode((uchar*)decoded.data() + leading_zeros, decoded.size());
	return decoded;
}

} /* namespace librevault */
