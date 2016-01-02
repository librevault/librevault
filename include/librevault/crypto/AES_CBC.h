/* Written in 2015 by Alexander Shishenko <alex@shishenko.com>
 *
 * LVCrypto - Cryptographic wrapper, used in Librevault.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#pragma once
#include "Transformer.h"
#include <cryptopp/aes.h>
#include <cryptopp/ccm.h>
#include <cryptopp/filters.h>

namespace librevault {
namespace crypto {

class AES_CBC : public TwoWayTransformer {
private:
	const blob& key;
	const blob& iv;
	bool padding;
public:
	AES_CBC(const blob& key, const blob& iv, bool padding = true) : key(key), iv(iv), padding(padding) {}
	virtual ~AES_CBC() {};

	blob encrypt(const blob& plaintext) const {
		CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption filter(key.data(), key.size(), iv.data());

		std::string ciphertext;
		CryptoPP::StringSource(plaintext.data(), plaintext.size(), true,
							   new CryptoPP::StreamTransformationFilter(filter,
																		new CryptoPP::StringSink(ciphertext),
																		padding ? CryptoPP::StreamTransformationFilter::PKCS_PADDING : CryptoPP::StreamTransformationFilter::NO_PADDING
							   )
		);

		return blob(std::make_move_iterator(ciphertext.begin()), std::make_move_iterator(ciphertext.end()));
	}

	blob decrypt(const blob& ciphertext) const {
		CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption filter(key.data(), key.size(), iv.data());

		std::string plaintext;
		CryptoPP::StringSource(ciphertext.data(), ciphertext.size(), true,
							   new CryptoPP::StreamTransformationFilter(filter,
																		new CryptoPP::StringSink(plaintext),
																		padding ? CryptoPP::StreamTransformationFilter::PKCS_PADDING : CryptoPP::StreamTransformationFilter::NO_PADDING
							   )
		);

		return blob(std::make_move_iterator(plaintext.begin()), std::make_move_iterator(plaintext.end()));
	}

	virtual blob to(const blob& data) const {return encrypt(data);}
	virtual blob from(const blob& data) const {return decrypt(data);}
};

} /* namespace crypto */
} /* namespace librevault */
