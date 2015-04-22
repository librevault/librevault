/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "Directory.h"

namespace librevault {
namespace sync {

Directory::Directory(const fs::path& directory_path) : directory_path(directory_path), storage(directory_path) {
	max_secret = '0';
}
Directory::~Directory() {}

void Directory::set_Secret(const Secret& secret) {
	max_secret = secret.type;
	storage.set_aes_key();
}

} /* namespace sync */
} /* namespace librevault */
