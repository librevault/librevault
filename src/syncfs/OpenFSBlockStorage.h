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
#ifndef SRC_SYNCFS_OPENFSBLOCKSTORAGE_H_
#define SRC_SYNCFS_OPENFSBLOCKSTORAGE_H_

#include "../../contrib/cryptowrappers/cryptowrappers.h"
#include <boost/filesystem.hpp>
#include "../types.h"

namespace librevault {
namespace syncfs {

class FSBlockStorage;
class OpenFSBlockStorage {
	FSBlockStorage* parent;

	std::pair<blob, blob> get_both_blocks(const crypto::StrongHash& block_hash);	// Returns std::pair(plaintext, encrypted)
public:
	OpenFSBlockStorage(FSBlockStorage* parent);
	virtual ~OpenFSBlockStorage();

	blob get_encblock(const crypto::StrongHash& block_hash);
	blob get_block(const crypto::StrongHash& block_hash);

	// File assembler
	bool can_assemble_file(const fs::path& relpath);	// Returns whether it is worth trying to assemble file or not. It does not ensure, that file CAN be assembled.
	void assemble_file(const fs::path& relpath, bool delete_blocks = false);
	void disassemble_file(const fs::path& relpath, bool delete_file = false);
};

} /* namespace syncfs */
} /* namespace librevault */

#endif /* SRC_SYNCFS_OPENFSBLOCKSTORAGE_H_ */
