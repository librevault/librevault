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
#include "SignedMeta.h"

#include <utility>

namespace librevault {

SignedMeta::SignedMeta(Meta meta, const Secret& secret) {
  meta_ = std::make_shared<Meta>(std::move(meta));
  raw_meta_ = meta_->serialize();
  signature_ = secret.sign(raw_meta_);
}

SignedMeta::SignedMeta(QByteArray raw_meta, QByteArray signature, const Secret& secret, bool check_signature)
    : raw_meta_(std::move(raw_meta)), signature_(std::move(signature)) {
  if (check_signature && secret.verify(raw_meta_, signature_)) throw SignatureError();

  meta_ = std::make_shared<Meta>(raw_meta_);
}

}  // namespace librevault
