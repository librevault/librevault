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

#include "Secret.h"
#include "crypto/HMAC-SHA3.h"

namespace librevault {

inline std::vector<uint8_t> derive_token(const Secret& secret, const std::vector<uint8_t>& cert_digest) {
	return cert_digest | crypto::HMAC_SHA3_224(secret.get_Public_Key());
}

} /* namespace librevault */
