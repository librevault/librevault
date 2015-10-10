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
#include "../Session.h"
#include "Abstract.h"
#include "Exchanger.h"

namespace librevault {

AbstractDirectory::AbstractDirectory(Session& session, Exchanger& exchanger) :
		session_(session), exchanger_(exchanger), log_(session_.log()) {}
AbstractDirectory::~AbstractDirectory() {}

std::string AbstractDirectory::path_id_readable(const blob& path_id) const {
	return crypto::Base32().to_string(path_id);
}

std::string AbstractDirectory::encrypted_data_hash_readable(const blob& block_id) const {
	return crypto::Base32().to_string(block_id);
}

} /* namespace librevault */
