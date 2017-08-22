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
#pragma once
#include "SecretPrivate.h"
#include <QByteArray>
#include <QReadWriteLock>
#include <QString>
#include <iostream>
#include <stdexcept>
#include <QSharedData>

namespace librevault {

class Secret {
 public:
  enum Type : char {
    Owner = 'A',      /// Not used now. Will be useful for 'managed' shares for security-related actions. Now equal to ReadWrite.
    ReadWrite = 'B',  /// Signature key, used to sign modified files.
    ReadOnly = 'C',   /// Encryption key (AES-256), used to encrypt blocks/filepaths and used in filepath HMAC
    Download = 'D',   /// Public key, used to verify signed modified files
  };

  struct error : public std::runtime_error {
    error(const char* what) : std::runtime_error(what) {}
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
  Secret(const QByteArray& string_secret);
  Secret(const QString& string_secret) : Secret(string_secret.toLatin1()) {}

  operator QString() const { return QString::fromLatin1(d->secret_s); }

  Type getType() const { return (Type)d->secret_s.at(0); }
  char getTypeChar() const { return (char)getType(); }
  char getParam() const { return d->secret_s[1]; }
  char getCheckChar() const { return d->secret_s[d->secret_s.size() - 1]; }

  // Secret derivers
  Secret derive(Type key_type) const;

  // Payload getters
  QByteArray getPrivateKey() const;
  QByteArray getPublicKey() const;
  QByteArray getEncryptionKey() const;

  QByteArray getHash() const;

  bool operator==(const Secret& key) const { return d->secret_s == key.d->secret_s; }
  bool operator<(const Secret& key) const { return d->secret_s < key.d->secret_s; }

 private:
  mutable QSharedDataPointer<SecretPrivate> d;

  Secret(Type type, QByteArray payload);

  QByteArray getEncodedPayload() const;
  QByteArray getPayload() const;
};

std::ostream& operator<<(std::ostream& os, const Secret& k);

} /* namespace librevault */
