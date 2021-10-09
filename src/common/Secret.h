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
 */
#pragma once
#include <QByteArray>
#include <QString>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace librevault {

class Secret {
 public:
  enum Type : char {
    Owner = 'A',  /// Not used now. Will be useful for 'managed' shares for security-related actions. Now equal to
    /// ReadWrite.
    ReadWrite = 'B',  /// Signature key, used to sign modified files.
    ReadOnly = 'C',   /// Encryption key (AES-256), used to encrypt blocks/filepaths and used in filepath HMAC
    Download = 'D',   /// Public key, used to verify signed modified files
  };

  struct error : public std::runtime_error {
    error(const char *what) : std::runtime_error(what) {}
  };
  struct format_error : public error {
    format_error() : error("Secret format mismatch") {}
  };
  struct level_error : public error {
    level_error() : error("Secret has insufficient privileges for this action") {}
  };
  struct crypto_error : public error {
    crypto_error() : error("Cryptographic error. Probably ECDSA domain mismatch") {}
  };

  Secret();
  Secret(Type type, const QByteArray &payload);
  Secret(const QString &str);

  QString string() const { return secret_s; }
  operator QString() const { return secret_s; }

  Type get_type() const { return (Type)secret_s.front(); }
  char get_param() const { return secret_s[1]; }
  char get_check_char() const { return secret_s.back(); }

  // Secret derivers
  Secret derive(Type key_type) const;

  // Payload getters
  QByteArray get_Private_Key() const;
  QByteArray get_Public_Key() const;
  QByteArray get_Encryption_Key() const;

  QByteArray get_Hash() const;

  bool operator==(const Secret &key) const { return secret_s == key.secret_s; }
  bool operator<(const Secret &key) const { return secret_s < key.secret_s; }

  QByteArray sign(const QByteArray& message) const;
  bool verify(const QByteArray& message, const QByteArray& signature) const;

 private:
  static constexpr size_t private_key_size = 32;
  static constexpr size_t encryption_key_size = 32;
  static constexpr size_t public_key_size = 33;

  static constexpr size_t hash_size = 32;

  QByteArray secret_s;

  mutable QByteArray cached_private_key;     // ReadWrite
  mutable QByteArray cached_encryption_key;  // ReadOnly
  mutable QByteArray cached_public_key;      // Download

  // It is a hash of Download key, used for searching for new nodes (e.g. in DHT) without leaking Download key.
  // Completely public, no need to hide it.
  mutable QByteArray cached_hash;

  QByteArray getEncodedPayload() const;
  QByteArray getPayload() const;
};

std::ostream &operator<<(std::ostream &os, const Secret &k);

}  // namespace librevault
