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
#ifndef SRC_CRYPTOFS_BLOCKSTORAGE_H_
#define SRC_CRYPTOFS_BLOCKSTORAGE_H_

#include "../util.h"
#include <cryptodiff.h>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

namespace librevault {

class BlockStorage {
public:
	BlockStorage();
	virtual ~BlockStorage();

	virtual cryptodiff::FileMap get_FileMap(const boost::filesystem::path& filepath) = 0;
//	cryptodiff::FileMap get_FileMap(const boost::filesystem::path& filepath, std::string& signature);
	virtual void put_FileMap(const boost::filesystem::path& filepath,
			const cryptodiff::FileMap& filemap,
			const std::string& signature,
			boost::optional<bool> force_ready = boost::none) = 0;

	virtual std::vector<uint8_t> get_block(const std::array<uint8_t, SHASH_LENGTH>& block_hash, cryptodiff::Block& block_meta) = 0;
	virtual void put_block(const std::array<uint8_t, SHASH_LENGTH>& block_hash, const std::vector<uint8_t>& data) = 0;
};

} /* namespace librevault */

#endif /* SRC_CRYPTOFS_BLOCKSTORAGE_H_ */
