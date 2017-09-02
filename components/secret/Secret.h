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
#include <QString>
#include <iostream>
#include <stdexcept>
#include <QSharedDataPointer>

namespace librevault {

class Secret {
 public:
  enum Level : char {
    Owner = 'A',      /// Not used now. Will be useful for 'managed' shares for security-related actions. Now equal to ReadWrite.
    ReadWrite = 'B',  /// Signature key, used to sign modified files.
    ReadOnly = 'C',   /// Encryption key (AES-256), used to encrypt blocks/filepaths and used in filepath HMAC
    Download = 'D',   /// Public key, used to verify signed modified files
  };

  struct error : public std::runtime_error {
    error(const char* what) : std::runtime_error(what) {}
  };
  struct FormatMismatch : public error {
    FormatMismatch() : error("Secret format mismatch") {}
  };
  struct LevelError : public error {
    LevelError() : error("Secret has insufficient privileges for this action") {}
  };
  struct CryptoError : public error {
    CryptoError() : error("Cryptographic error. Probably ECDSA domain mismatch") {}
  };
  struct UnknownSecretLevel : public error {
    UnknownSecretLevel() : error("Unknown Secret level") {}
  };
  struct UnknownSecretVersion : public error {
    UnknownSecretVersion() : error("Unknown Secret version") {}
  };

  Secret();
  Secret(const QByteArray& string_secret);
  Secret(const QString& string_secret) : Secret(string_secret.toLatin1()) {}

  static Secret generate();

  QString toString() const;
  operator QString() const { return toString(); }

  // Parameters
  Level level() const { return (Level)d->level_; }
  char version() const { return d->version_; }

  // Payload getters
  const QByteArray& privateKey() const;
  const QByteArray& publicKey() const;
  const QByteArray& encryptionKey() const;

  const QByteArray& folderid() const;

  // Secret derivers
  Secret derive(Level key_type) const;

  bool operator==(const Secret& key) const { return toString() == key.toString(); }
  bool operator<(const Secret& key) const { return toString() < key.toString(); }

 private:
  QSharedDataPointer<SecretPrivate> d;

  QByteArray makeBinaryPayload() const;
  bool currentLevelLessThan(Level lvl) const;
};

std::ostream& operator<<(std::ostream& os, const Secret& k);

} /* namespace librevault */
