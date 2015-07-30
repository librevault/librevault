/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "../../contrib/lvsqlite3/SQLiteWrapper.h"
#include "../Abstract.h"

namespace librevault {

class Index {
public:
	using SignedMeta = AbstractDirectory::SignedMeta;

	class no_such_meta : public std::runtime_error {
	public:
		no_such_meta() : std::runtime_error("Requested Meta not found"){}
	};

	Index(fs::path db_path);
	virtual ~Index();

	void wipe();

	void put_Meta(std::list<SignedMeta> signed_meta_list);
	void put_Meta(SignedMeta signed_meta){put_Meta(std::list<SignedMeta>{signed_meta});}

	SignedMeta get_Meta(blob path_hmac);
	std::list<SignedMeta> get_Meta(int64_t mtime);
	std::list<SignedMeta> get_Meta();

	SQLiteDB& db() {return *db_;}

	fs::path db_path() const {return db_path_;}

private:
	std::shared_ptr<spdlog::logger> log_;
	std::unique_ptr<SQLiteDB> db_;	// Better use SOCI library ( https://github.com/SOCI/soci ). My "reinvented wheel" isn't stable enough.

	const fs::path db_path_;

	std::list<SignedMeta> get_Meta(std::string sql, std::map<std::string, SQLValue> values = std::map<std::string, SQLValue>());
};

} /* namespace librevault */
