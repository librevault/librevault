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
#ifndef SRC_SYNCFS_ENCFSBLOCKSTORAGE_H_
#define SRC_SYNCFS_ENCFSBLOCKSTORAGE_H_

#include "syncfs.h"
#include "Meta.pb.h"
#include "../../contrib/cryptowrappers/cryptowrappers.h"
#include "../../contrib/lvsqlite3/SQLiteWrapper.h"

namespace librevault {
namespace syncfs {

class FSBlockStorage;
class EncFSBlockStorage {
	FSBlockStorage* parent;

	fs::path make_encblock_path(const crypto::StrongHash& block_hash);
public:
	EncFSBlockStorage(FSBlockStorage* parent);
	virtual ~EncFSBlockStorage();

	bool verify_encblock(const crypto::StrongHash& block_hash, const blob& data);

	bool have_encblock(const crypto::StrongHash& block_hash);
	blob get_encblock(const crypto::StrongHash& block_hash);
	void put_encblock(const crypto::StrongHash& block_hash, const blob& data);
	void remove_encblock(const crypto::StrongHash& block_hash);
};

} /* namespace syncfs */
} /* namespace librevault */

#endif /* SRC_SYNCFS_ENCFSBLOCKSTORAGE_H_ */
