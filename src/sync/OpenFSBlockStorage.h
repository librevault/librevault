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
#ifndef SRC_SYNC_OPENFSBLOCKSTORAGE_H_
#define SRC_SYNC_OPENFSBLOCKSTORAGE_H_

#include "EncFSBlockStorage.h"
#include <sqlite3.h>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

namespace librevault {

class OpenFSBlockStorage : public EncFSBlockStorage {
	cryptodiff::Key encryption_key;

	//
	boost::optional<boost::filesystem::path> relpath(boost::filesystem::path path) const;

	void create_index_file(const boost::filesystem::path& filepath);
	void update_index_file(const boost::filesystem::path& filepath);

	std::vector<uint8_t> sign(const FileMeta& file_meta);
protected:
public:
	OpenFSBlockStorage(const boost::filesystem::path& dirpath, const boost::filesystem::path& dbpath, const cryptodiff::Key& key);
	virtual ~OpenFSBlockStorage();

	void create_index();
	void update_index();

	virtual int64_t put_FileMeta(const FileMeta& meta, const std::vector<uint8_t>& signature);
	virtual FileMeta get_FileMeta(const boost::filesystem::path& filepath, std::vector<uint8_t>& signature);
	using EncFSBlockStorage::get_FileMeta;

	virtual std::vector<uint8_t> get_block_data(const cryptodiff::StrongHash& block_hash);
	virtual void put_block_data(const cryptodiff::StrongHash& block_hash, const std::vector<uint8_t>& data);
};

} /* namespace librevault */

#endif /* SRC_SYNC_OPENFSBLOCKSTORAGE_H_ */
