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
#ifndef SRC_SYNCFS_METASTORAGE_H_
#define SRC_SYNCFS_METASTORAGE_H_

#include "syncfs.h"
#include "Meta.pb.h"
#include "../../contrib/cryptowrappers/cryptowrappers.h"
#include "../../contrib/lvsqlite3/SQLiteWrapper.h"

namespace librevault {
namespace syncfs {

class FSBlockStorage;
class MetaStorage {
	FSBlockStorage* parent;

	bool have_path = false;
public:
	struct SignedMeta {
		Meta meta;
		blob signature;
	};
public:
	MetaStorage(FSBlockStorage* parent);
	virtual ~MetaStorage();

	void set_key(const crypto::Key& aes_key);
	void reset_key();

	// Encrypted path operations
	bool have_Meta(const blob& encpath);
	SignedMeta get_Meta(const blob& encpath);
	void remove_Meta(const blob& encpath);

	// Non-encrypted path operations
	bool have_Meta(const fs::path& relpath);
	SignedMeta get_Meta(const fs::path& relpath);
	void remove_Meta(const fs::path& relpath);

	void put_Meta(const SignedMeta& meta);

	std::list<SignedMeta> get_custom(const std::string& sql, std::map<std::string, SQLValue> values = std::map<std::string, SQLValue>());
	std::list<SignedMeta> get_all();
	std::list<SignedMeta> get_newer_than(int64_t mtime);
};

} /* namespace syncfs */
} /* namespace librevault */

#endif /* SRC_SYNCFS_METASTORAGE_H_ */
