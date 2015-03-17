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
#ifndef SRC_SYNC_ENCFSBLOCKSTORAGE_H_
#define SRC_SYNC_ENCFSBLOCKSTORAGE_H_

#include <cryptodiff.h>
#include <sqlite3.h>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

namespace librevault {

class EncFSBlockStorage {
	boost::filesystem::path encblocks_path;
	boost::filesystem::path directory_path;
	sqlite3* directory_db = 0;
public:
	EncFSBlockStorage(const boost::filesystem::path& dirpath, const boost::filesystem::path& dbpath);
	virtual ~EncFSBlockStorage();

	cryptodiff::EncFileMap get_EncFileMap(const boost::filesystem::path& filepath);
//	cryptodiff::FileMap get_FileMap(const boost::filesystem::path& filepath, std::string& signature);
	/**
	 * Writes FileMap to database.
	 * @param filepath
	 * @param filemap
	 * @param signature
	 * @param force_ready Sets "ready" flag for specific block. If not set (or equal to boost::none) this function will check if blocks are ready. For this it will create new filemap internally, which is really io- and cpu-heavy process.
	 */
	void put_EncFileMap(const boost::filesystem::path& filepath,
			const cryptodiff::EncFileMap& filemap,
			const std::string& signature,
			boost::optional<bool> force_ready = boost::none);

	std::vector<uint8_t> get_block(const cryptodiff::shash_t& block_hash, cryptodiff::Block& block_meta);
	void put_block(const cryptodiff::shash_t& block_hash, const std::vector<uint8_t>& data);
};

} /* namespace librevault */

#endif /* SRC_SYNC_ENCFSBLOCKSTORAGE_H_ */
