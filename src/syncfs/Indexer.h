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
#ifndef SRC_SYNCFS_INDEXER_H_
#define SRC_SYNCFS_INDEXER_H_

#include "MetaStorage.h"
#include "FileMeta.pb.h"
#include <boost/filesystem.hpp>

namespace librevault {
namespace syncfs {

class FSBlockStorage;
class Indexer {
	FSBlockStorage* parent;

	inline std::string make_filetype_string(Meta::FileType filetype) const {
		switch(filetype){
			case Meta::FileType::Meta_FileType_FILE:
				return "FILE";
			case Meta::FileType::Meta_FileType_DIRECTORY:
				return "DIRECTORY";
			case Meta::FileType::Meta_FileType_SYMLINK:
				return "SYMLINK";
			default:
				return "UNKNOWN";
		}
	}

	Meta make_created_Meta(const fs::path& relpath, bool without_filemap=false);
	Meta make_updated_Meta(const fs::path& relpath);
	Meta make_deleted_Meta(const fs::path& relpath);

	MetaStorage::SignedMeta make_signature(const Meta& meta);
	boost::optional<fs::path> make_relpath(const fs::path& path) const;
public:
	Indexer(FSBlockStorage* parent);
	virtual ~Indexer();

	void create_index();
	void update_index();

	void create_index_file(const fs::path& relpath);
	void update_index_file(const fs::path& relpath);
	void delete_index_file(const fs::path& relpath);
};

} /* namespace syncfs */
} /* namespace librevault */

#endif /* SRC_SYNCFS_INDEXER_H_ */
