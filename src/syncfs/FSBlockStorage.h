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
#ifndef SRC_SYNCFS_FSBLOCKSTORAGE_H_
#define SRC_SYNCFS_FSBLOCKSTORAGE_H_

#include "MetaStorage.h"
#include "Indexer.h"
#include "EncFSBlockStorage.h"
#include "OpenFSBlockStorage.h"

#include "syncfs.h"

namespace librevault {
namespace syncfs {

class FSBlockStorage {
	// Shared DB
	std::shared_ptr<SQLiteDB> directory_db;

	// Storages
	friend class MetaStorage; std::shared_ptr<MetaStorage> meta_storage;
	friend class Indexer; std::shared_ptr<Indexer> indexer;
	friend class EncFSBlockStorage; std::shared_ptr<EncFSBlockStorage> encfs_block_storage;
	friend class OpenFSBlockStorage; std::shared_ptr<OpenFSBlockStorage> openfs_block_storage;

	// Paths
	fs::path system_path;
	fs::path open_path;

	// Encryption
	crypto::Key aes_key;
public:
	enum PresentLocations {
		ENCFS = 1,
		OPENFS = 2,
		MISSING = 0
	};
	enum Errors {
		NoSuchMeta,
		NoSuchBlock,
		NotEnoughBlocks
	};
public:
	FSBlockStorage(const fs::path& dirpath);
	virtual ~FSBlockStorage();

	const crypto::Key& get_aes_key() const {return aes_key;}
};

} /* namespace syncfs */
} /* namespace librevault */

#endif /* SRC_SYNCFS_FSBLOCKSTORAGE_H_ */
