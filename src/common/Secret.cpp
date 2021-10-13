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

#include "util/ffi.h"

namespace librevault {

Secret::Secret() : opaque_secret_(secret_new()) {}

Secret::Secret(const QString& str) : opaque_secret_(secret_from_str(rust::Str(str.toStdString()))) {}

Secret::~Secret() = default;

Secret::Secret(const Secret& secret) : opaque_secret_(secret.opaque_secret_->c_clone()) {}

Secret::Secret(rust::Box<OpaqueSecret>&& opaque_secret) : opaque_secret_(std::move(opaque_secret)) {}

Secret& Secret::operator=(const Secret &secret) {
  opaque_secret_ = secret.opaque_secret_->c_clone();
  return *this;
}

Secret Secret::derive(Type key_type) const {
  return {opaque_secret_->c_derive(key_type)};
}

QByteArray Secret::get_Private_Key() const {
  return from_vec(opaque_secret_->c_get_private());
}

QByteArray Secret::get_Encryption_Key() const {
  return from_vec(opaque_secret_->c_get_symmetric());
}

QByteArray Secret::get_Public_Key() const {
  return from_vec(opaque_secret_->c_get_public());
}

QString Secret::string() const { return QString::fromStdString(std::string(opaque_secret_->to_string())); }

QByteArray Secret::get_Hash() const {
  if (!cached_hash.isEmpty()) return cached_hash;
  return cached_hash = QCryptographicHash::hash(get_Public_Key(), QCryptographicHash::Algorithm::Sha3_256);
}

QByteArray Secret::sign(const QByteArray& message) const {
  return from_vec(opaque_secret_->sign(to_slice(message)));
}

bool Secret::verify(const QByteArray& message, const QByteArray& signature) const {
  return opaque_secret_->verify(to_slice(message), to_slice(signature));
}

std::ostream& operator<<(std::ostream& os, const Secret& k) { return os << k.string().toStdString(); }

}  // namespace librevault
