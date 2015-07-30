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
#include "../../pch.h"
#include "AbstractStorage.h"

namespace librevault {

class EncStorage : public AbstractStorage {
public:
	EncStorage(fs::path block_path);
	virtual ~EncStorage();

	bool have_encblock(const blob& block_hash);
	blob get_encblock(const blob& block_hash);
	void put_encblock(const blob& block_hash, const blob& data);
	void remove_encblock(const blob& block_hash);

	fs::path block_path() const {return block_path_;}

private:
	std::shared_ptr<spdlog::logger> log_;
	fs::path block_path_;

	fs::path make_encblock_name(const blob& block_hash) const;
	fs::path make_encblock_path(const blob& block_hash) const;
};

} /* namespace librevault */
