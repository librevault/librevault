/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "src/pch.h"
#include <librevault/SignedMeta.h>
#include "src/util/Loggable.h"

namespace librevault {

class Client;
class FSFolder;

class Index : public Loggable {
public:
	Index(FSFolder& dir);
	virtual ~Index() {}

	/* Meta manipulators */
	SignedMeta get_Meta(const blob& path_id);
	std::list<SignedMeta> get_Meta();

	void put_Meta(const SignedMeta& signed_meta, bool fully_assembled);

	/* Chunk getter */
	uint32_t get_chunk_size(const blob& ct_hash);

	/* Properties */
	std::list<SignedMeta> containing_chunk(const blob& ct_hash);
	SQLiteDB& db() {return *db_;}

private:
	FSFolder& dir_;

	std::unique_ptr<SQLiteDB> db_;	// Better use SOCI library ( https://github.com/SOCI/soci ). My "reinvented wheel" isn't stable enough.

	std::list<SignedMeta> get_Meta(std::string sql, std::map<std::string, SQLValue> values = std::map<std::string, SQLValue>());
	void wipe();
};

} /* namespace librevault */
