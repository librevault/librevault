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
#ifndef SRC_SYNC_DIRECTORY_H_
#define SRC_SYNC_DIRECTORY_H_

#include "../types.h"
#include <boost/optional.hpp>

namespace librevault {
namespace sync {

class Directory {
	fs::path directory_path;
	syncfs::FSBlockStorage storage;

	struct Secret {
		const std::array<char, 4> magic = {'S', 'Y', 'N', 'C'};	// 4 bytes
		enum Type : char {
			ReadWrite = 'A',
			ReadOnly = 'B',
			Untrusted = 'C'
		} type;	// 1 byte
		std::array<char, crypto::AES_KEYSIZE*2> key;	// 64 bytes, as AES-32 key will be 64 bytes in Base32.
		char check_digit;	// 1 byte
	};	// 70 bytes

	Secret download_secret;

	Secret::Type max_secret;
public:
	Directory(const fs::path& directory_path);
	virtual ~Directory();

	void set_Secret(const Secret& secret);
};

} /* namespace sync */
} /* namespace librevault */

#endif /* SRC_SYNC_DIRECTORY_H_ */
