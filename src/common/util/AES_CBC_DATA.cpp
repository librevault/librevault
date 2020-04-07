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
#include "AES_CBC_DATA.h"

#include "Meta.h"
#include "crypto/AES_CBC.h"

namespace librevault {

bool AES_CBC_DATA::check(const Secret& secret) {
  if (!check()) return false;
  try {
    ct | crypto::De<crypto::AES_CBC>(conv_bytearray(secret.get_Encryption_Key()), iv);
  } catch (const CryptoPP::Exception& e) {
    return false;
  }
  return true;
}

std::vector<uint8_t> AES_CBC_DATA::get_plain(const Secret& secret) const {
  try {
    return ct | crypto::De<crypto::AES_CBC>(conv_bytearray(secret.get_Encryption_Key()), iv);
  } catch (const CryptoPP::Exception& e) {
    throw Meta::parse_error("Parse error: Decryption failed");
  }
}

void AES_CBC_DATA::set_plain(const std::vector<uint8_t>& pt, const Secret& secret) {
  iv = crypto::AES_CBC::random_iv();
  ct = pt | crypto::AES_CBC(conv_bytearray(secret.get_Encryption_Key()), iv);
}

}  // namespace librevault
