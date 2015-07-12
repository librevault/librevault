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
#pragma once

#include "../types.h"
#include <spdlog/spdlog.h>

namespace librevault {
namespace syncfs {

class EncStorage {
	const fs::path& block_path;

	fs::path make_encblock_name(const blob& block_hash) const;
	fs::path make_encblock_path(const blob& block_hash) const;

	// Logger
	std::shared_ptr<spdlog::logger> log;
public:
	EncStorage(const fs::path& block_path);
	virtual ~EncStorage();

	bool verify_encblock(const blob& block_hash, const blob& data);

	bool have_encblock(const blob& block_hash);
	blob get_encblock(const blob& block_hash);
	void put_encblock(const blob& block_hash, const blob& data);
	void remove_encblock(const blob& block_hash);
};

} /* namespace syncfs */
} /* namespace librevault */
