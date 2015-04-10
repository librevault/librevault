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
#include "util.h"
#include <cryptopp/sha3.h>
#include <cryptopp/filters.h>
#include <cryptopp/aes.h>
#include <cryptopp/ccm.h>
#include <cryptopp/hex.h>
#include <cryptopp/base32.h>
#include <cryptopp/hmac.h>

#include <boost/algorithm/string.hpp>

#include <sstream>
#include <iomanip>

namespace librevault {

uint64_t filesize(std::istream& ifile){
	auto cur_pos = ifile.tellg();
	ifile.seekg(0, ifile.end);
	auto size = ifile.tellg();
	ifile.seekg(cur_pos);
	return size;
}

uint64_t filesize(std::ostream& ofile){
	auto cur_pos = ofile.tellp();
	ofile.seekp(0, ofile.end);
	auto size = ofile.tellp();
	ofile.seekp(cur_pos);
	return size;
}

std::vector<uint8_t> encrypt(const uint8_t* data, size_t size, key_t key, iv_t iv, bool nopad) {
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

std::vector<uint8_t> decrypt(const uint8_t* data, size_t size, key_t key, iv_t iv, bool nopad) {
	if(data == nullptr || size == 0) return std::vector<uint8_t>();
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

std::string to_hex(const uint8_t* data, size_t size){
	std::string hexdata;
	CryptoPP::StringSource(data, size, true, new CryptoPP::HexEncoder( new CryptoPP::StringSink(hexdata)));

	boost::algorithm::to_lower(hexdata);

	return hexdata;
}

std::string to_hex(const std::string& s){
	return to_hex((uint8_t*)s.data(), s.size());
}

std::string to_hex(const uint32_t& s){
	std::ostringstream ret;
	ret << "0x" << std::hex << std::setfill('0') << std::setw(8) << s;

	return ret.str();
}

std::string to_base32(const uint8_t* data, size_t size){
	std::string base32data;
	CryptoPP::StringSource(data, size, true, new CryptoPP::Base32Encoder(new CryptoPP::StringSink(base32data)));
	boost::algorithm::to_lower(base32data);

	return base32data;
}

std::vector<uint8_t> from_base32(const std::string& data){
	std::string base32data;
	CryptoPP::StringSource(data, true, new CryptoPP::Base32Encoder(new CryptoPP::StringSink(base32data)));

	return std::vector<uint8_t>(std::move(base32data.begin()), std::move(base32data.end()));
}

shash_t compute_shash(const uint8_t* data, size_t size) {
	CryptoPP::SHA3_224 hasher;

	std::array<uint8_t, SHASH_LENGTH> hash;
	hasher.CalculateDigest(hash.data(), data, size);

	return hash;
}

shash_t compute_hmac(const uint8_t* data, size_t size, key_t key){
	CryptoPP::HMAC<CryptoPP::SHA3_224> hmacer(key.data(), key.size());

	std::array<uint8_t, SHASH_LENGTH> hmac;
	hmacer.CalculateDigest(hmac.data(), data, size);

	return hmac;
}

} /* namespace librevault */
