/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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
#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#include <cryptopp/sha3.h>
#include <cryptopp/filters.h>
#include <cryptopp/aes.h>
#include <cryptopp/ccm.h>
#include <cryptopp/hex.h>

#include <boost/algorithm/string.hpp>

#include <sstream>
#include <iomanip>
#include <cstdint>
#include <iostream>
#include <string>

namespace librevault {

constexpr size_t SHASH_LENGTH = 28;
constexpr size_t AES_BLOCKSIZE = 16;
constexpr size_t AES_KEYSIZE = 32;

using key_t = std::array<uint8_t, AES_KEYSIZE>;
using iv_t = std::array<uint8_t, AES_BLOCKSIZE>;
using shash_t = std::array<uint8_t, SHASH_LENGTH>;

inline uint64_t filesize(std::istream& ifile){
	auto cur_pos = ifile.tellg();
	ifile.seekg(0, ifile.end);
	auto size = ifile.tellg();
	ifile.seekg(cur_pos);
	return size;
}

inline uint64_t filesize(std::ostream& ofile){
	auto cur_pos = ofile.tellp();
	ofile.seekp(0, ofile.end);
	auto size = ofile.tellp();
	ofile.seekp(cur_pos);
	return size;
}

inline std::vector<uint8_t> encrypt(const uint8_t* data, size_t size, std::array<uint8_t, AES_KEYSIZE> key, std::array<uint8_t, AES_BLOCKSIZE> iv, bool nopad = false) {
	CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryptor;
	encryptor.SetKeyWithIV(key.data(), key.size(), iv.data());

	std::vector<uint8_t> ciphertext( size - (size % AES_BLOCKSIZE) + AES_BLOCKSIZE );
	CryptoPP::StringSource str_source(data, size, true,
			new CryptoPP::StreamTransformationFilter(encryptor,
					new CryptoPP::ArraySink(ciphertext.data(), ciphertext.size()),
					nopad ? CryptoPP::StreamTransformationFilter::NO_PADDING : CryptoPP::StreamTransformationFilter::PKCS_PADDING
			)
	);

	return ciphertext;
}

inline std::vector<uint8_t> decrypt(const uint8_t* data, size_t size, std::array<uint8_t, AES_KEYSIZE> key, std::array<uint8_t, AES_BLOCKSIZE> iv, bool nopad = false) {
	CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryptor;
	decryptor.SetKeyWithIV(key.data(), key.size(), iv.data());

	std::string text;
	CryptoPP::StringSource(data, size, true,
			new CryptoPP::StreamTransformationFilter(decryptor,
					new CryptoPP::StringSink(text),
					nopad ? CryptoPP::StreamTransformationFilter::NO_PADDING : CryptoPP::StreamTransformationFilter::PKCS_PADDING
			)
	);

	return std::vector<uint8_t>(text.begin(), text.end());
}

inline std::string to_hex(const uint8_t* data, size_t size){
	std::string hexdata;
	CryptoPP::StringSource(data, size, true,
			new CryptoPP::HexEncoder(
					new CryptoPP::StringSink(hexdata)
			)
	);

	boost::algorithm::to_lower(hexdata);

	return hexdata;
}

inline std::string to_hex(const std::string& s){
	return to_hex((uint8_t*)s.data(), s.size());
}

inline std::string to_hex(const uint32_t& s){
	std::ostringstream ret;
	ret << "0x" << std::hex << std::setfill('0') << std::setw(8) << s;

	return ret.str();
}

inline std::array<uint8_t, SHASH_LENGTH> compute_shash(const uint8_t* data, size_t size) {
	CryptoPP::SHA3_224 hasher;

	std::array<uint8_t, SHASH_LENGTH> hash;
	hasher.CalculateDigest(hash.data(), data, size);

	return hash;
}

} /* namespace librevault */

#endif /* SRC_UTIL_H_ */
