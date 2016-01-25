/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "../../pch.h"
#include "AbstractStorage.h"

namespace librevault {

class Client;
class FSFolder;
class EncStorage : public AbstractStorage, public Loggable {
public:
	EncStorage(FSFolder& dir);
	virtual ~EncStorage() {}

	bool have_block(const blob& encrypted_data_hash) const;
	std::shared_ptr<blob> get_block(const blob& encrypted_data_hash) const;
	void put_block(const blob& encrypted_data_hash, const blob& data);	// FIXME: Check hash
	void remove_block(const blob& encrypted_data_hash);

private:
	const fs::path& block_path_;

	fs::path make_encblock_name(const blob& encrypted_data_hash) const;
	fs::path make_encblock_path(const blob& encrypted_data_hash) const;
};

} /* namespace librevault */
