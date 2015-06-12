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

#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>

#include <string>
#include <vector>
#include <stdexcept>

namespace librevault {
namespace syncfs {

class Key {
	static constexpr size_t private_key_size = 32;
	static constexpr size_t encryption_key_size = 32;
	static constexpr size_t public_key_size = 33;

	std::string secret_s;

	mutable std::vector<uint8_t> cached_private_key;	// ReadWrite
	mutable std::vector<uint8_t> cached_encryption_key;	// ReadOnly
	mutable std::vector<uint8_t> cached_public_key;		// Download

	std::vector<uint8_t> get_payload() const;
public:
	enum Type : char {
		Owner = 'A',	/// Not used now. Will be useful for 'managed' shares for security-related actions. Now equal to ReadWrite.
		ReadWrite = 'B',	/// Signature key, used to sign modified files.
		ReadOnly = 'C',	/// Encryption key (AES-256), used to encrypt blocks/filepaths and used in filepath HMAC
		Download = 'D'	/// Public key, used to verify signed modified files
	};

	class error : public std::domain_error {
	public:
		error(const char* what) : std::domain_error(what) {}
	};
	class format_error : public error {
	public:
		format_error() : error("Secret format mismatch") {}
	};
	class level_error : public error {
	public:
		level_error() : error("Key has insufficient privileges for this action") {}
	};
	class crypto_error : public error {
	public:
		crypto_error() : error("Cryptographic error. Probably ECDSA domain mismatch") {}
	};

public:	// Yes, I prefer splitting member variables and method declaration
	Key();
	Key(Type type, const std::vector<uint8_t>& payload);
	Key(std::string string_secret);

	std::string string() const {return secret_s;}
	operator std::string() const {return string();}

	Type getType() const {return (Type)secret_s.front();}
	char getCheckChar() const {return secret_s.back();}

	// Key derivers
	Key derive(Type key_type);

	// Payload getters
	std::vector<uint8_t>& get_Private_Key() const;
	std::vector<uint8_t>& get_Public_Key() const;
	std::vector<uint8_t>& get_Encryption_Key() const;

	virtual ~Key();

	bool operator== (const Key& key) const {return secret_s == key.secret_s;}
	bool operator< (const Key& key) const {return secret_s < key.secret_s;}
};

std::ostream& operator<<(std::ostream& os, const Key& k);

} /* namespace syncfs */
} /* namespace librevault */
