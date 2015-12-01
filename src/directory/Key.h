/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#pragma once

#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>

#include <string>
#include <vector>
#include <stdexcept>

namespace librevault {

class Key {
public:
	enum Type : char {
		Owner = 'A',	/// Not used now. Will be useful for 'managed' shares for security-related actions. Now equal to ReadWrite.
		ReadWrite = 'B',	/// Signature key, used to sign modified files.
		ReadOnly = 'C',	/// Encryption key (AES-256), used to encrypt blocks/filepaths and used in filepath HMAC
		Download = 'D',	/// Public key, used to verify signed modified files
	};

	struct error : public std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
	};
	struct format_error : public error {
		format_error() : error("Secret format mismatch") {}
	};
	struct level_error : public error {
		level_error() : error("Key has insufficient privileges for this action") {}
	};
	struct crypto_error : public error {
		crypto_error() : error("Cryptographic error. Probably ECDSA domain mismatch") {}
	};

	Key();
	Key(Type type, const std::vector<uint8_t>& payload);
	Key(std::string string_secret);

	std::string string() const {return secret_s;}
	operator std::string() const {return string();}

	Type get_type() const {return (Type)secret_s.front();}
	char get_check_char() const {return secret_s.back();}

	// Key derivers
	Key derive(Type key_type) const;

	// Payload getters
	const std::vector<uint8_t>& get_Private_Key() const;
	const std::vector<uint8_t>& get_Public_Key() const;
	const std::vector<uint8_t>& get_Encryption_Key() const;

	const std::vector<uint8_t>& get_Hash() const;

	bool operator== (const Key& key) const {return secret_s == key.secret_s;}
	bool operator< (const Key& key) const {return secret_s < key.secret_s;}

private:
	static constexpr size_t private_key_size = 32;
	static constexpr size_t encryption_key_size = 32;
	static constexpr size_t public_key_size = 33;

	static constexpr size_t hash_size = 32;

	std::string secret_s;

	mutable std::vector<uint8_t> cached_private_key;	// ReadWrite
	mutable std::vector<uint8_t> cached_encryption_key;	// ReadOnly
	mutable std::vector<uint8_t> cached_public_key;		// Download

	mutable std::vector<uint8_t> cached_hash;			// It is a hash of Download key, used for searching for new nodes (e.g. in DHT) without leaking Download key. Completely public, no need to hide it.

	std::vector<uint8_t> get_payload() const;
};

std::ostream& operator<<(std::ostream& os, const Key& k);

} /* namespace librevault */
