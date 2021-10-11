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
#include "Secret.h"

#include <QCryptographicHash>
#include <crypto/Base58.h>


namespace librevault {

Secret::Secret() : opaque_secret_(secret_new(), secret_destroy) {}

Secret::Secret(OpaqueSecret* opaque_secret) : opaque_secret_(opaque_secret, secret_destroy) {}

Secret::Secret(const QString& str) : Secret(secret_from_string(str.toLatin1().data())) {}

Secret::~Secret() = default;

Secret::Secret(const Secret& secret): Secret(secret_clone(secret.opaque_secret_.get())) {}

Secret& Secret::operator=(const Secret &secret) {
  opaque_secret_.reset(secret_clone(secret.opaque_secret_.get()));
  return *this;
}

Secret Secret::derive(Type key_type) const {
  return {secret_derive(opaque_secret_.get(), key_type)};
}

QByteArray Secret::get_Private_Key() const {
  return from_rust(secret_get_private(opaque_secret_.get()));
}

QByteArray Secret::get_Encryption_Key() const {
  return from_rust(secret_get_symmetric(opaque_secret_.get()));
}

QByteArray Secret::get_Public_Key() const {
  return from_rust(secret_get_public(opaque_secret_.get()));
}

QString Secret::string() const { return QString::fromLatin1(from_rust(secret_as_string(opaque_secret_.get()))); }

QByteArray Secret::get_Hash() const {
  if (!cached_hash.isEmpty()) return cached_hash;
  return cached_hash = QCryptographicHash::hash(get_Public_Key(), QCryptographicHash::Algorithm::Sha3_256);
}

QByteArray Secret::sign(const QByteArray& message) const {
  return from_rust(secret_sign(opaque_secret_.get(), from_cpp(message)));
}

bool Secret::verify(const QByteArray& message, const QByteArray& signature) const {
  return secret_verify(opaque_secret_.get(), from_cpp(message), from_cpp(signature));
}

std::ostream& operator<<(std::ostream& os, const Secret& k) { return os << k.string().toStdString(); }

}  // namespace librevault
