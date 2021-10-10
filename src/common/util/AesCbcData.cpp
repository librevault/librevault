/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
#include "AesCbcData.h"

#include "Meta.h"
#include "crypto/AES_CBC.h"

namespace librevault {

QByteArray AesCbcData::ct() const { return ct_; }
QByteArray AesCbcData::iv() const { return iv_; }

void AesCbcData::setEncrypted(const std::string& ct, const std::string& iv) {
  ct_ = QByteArray::fromStdString(ct);
  iv_ = QByteArray::fromStdString(iv);
}

QByteArray AesCbcData::get_plain(const Secret& secret) const {
  return ct_ | crypto::De<crypto::AES_CBC>(secret.get_Encryption_Key(), iv_);
}

void AesCbcData::set_plain(const QByteArray& pt, const Secret& secret) {
  iv_ = crypto::AES_CBC::randomIv();
  ct_ = pt | crypto::AES_CBC(secret.get_Encryption_Key(), iv_);
}

}  // namespace librevault
