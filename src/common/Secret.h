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
#include <librevault_util/src/secret.rs.h>

#include <QByteArray>
#include <QString>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace librevault {

class Secret {
 public:
  enum Type : char {
    Owner = 'A',  /// Signature key, used to sign modified files.
    ReadOnly = 'B',   /// Encryption key (AES-256), used to encrypt blocks/filepaths and used in filepath HMAC
    Download = 'C',   /// Public key, used to verify signed modified files
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
  Secret(const QString &str);
  Secret(const Secret& secret);
  ~Secret();

  QString string() const;
  operator QString() const { return string(); }

  Type get_type() const { return (Type)string().front().toLatin1(); }

  // Secret derivers
  Secret derive(Type key_type) const;

  // Payload getters
  QByteArray get_Private_Key() const;
  QByteArray get_Public_Key() const;
  QByteArray get_Encryption_Key() const;

  QByteArray get_Hash() const;

  bool operator==(const Secret &key) const { return string() == key.string(); }
  bool operator<(const Secret &key) const { return string() < key.string(); }

  QByteArray sign(const QByteArray& message) const;
  bool verify(const QByteArray& message, const QByteArray& signature) const;

  Secret& operator=(const Secret &secret);

 private:
  QByteArray secret_s;

  // It is a hash of Download key, used for searching for new nodes (e.g. in DHT) without leaking Download key.
  // Completely public, no need to hide it.
  mutable QByteArray cached_hash;

  Secret(rust::Box<OpaqueSecret>&& opaque_secret);

  rust::Box<OpaqueSecret> opaque_secret_;
};

std::ostream &operator<<(std::ostream &os, const Secret &k);

}  // namespace librevault
