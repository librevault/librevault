/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "Meta.h"

namespace librevault {

Meta::Meta() {}
Meta::~Meta() {}

std::string Meta::gen_relpath(const Key& key) {
	blob result = encrypted_path_ | crypto::De<crypto::AES_CBC>(key.get_Encryption_Key(), encrypted_path_iv_);
	return std::string(std::make_move_iterator(result.begin()), std::make_move_iterator(result.end()));
}

blob Meta::serialize() const {

}

void Meta::parse(const blob &serialized_data) {

}

} /* namespace librevault */
