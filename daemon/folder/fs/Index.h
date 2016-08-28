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
#include "pch.h"
#include <librevault/SignedMeta.h>
#include "util/Loggable.h"
#include "util/SQLiteWrapper.h"
#include <boost/signals2/signal.hpp>

namespace librevault {

class Client;
class FSFolder;

class Index : public Loggable {
public:
	struct status_t {
		uint64_t file_entries = 0;
		uint64_t directory_entries = 0;
		uint64_t symlink_entries = 0;
		uint64_t deleted_entries = 0;
	};

	boost::signals2::signal<void(const SignedMeta&)> new_meta_signal;
	boost::signals2::signal<void(const Meta&)> assemble_meta_signal;

	Index(FSFolder& dir, Client& client);
	virtual ~Index() {}

	/* Meta manipulators */
	bool have_meta(const Meta::PathRevision& path_revision) noexcept;
	SignedMeta get_meta(const Meta::PathRevision& path_revision);
	SignedMeta get_meta(const blob& path_id);
	std::list<SignedMeta> get_meta();
	std::list<SignedMeta> get_existing_meta();
	std::list<SignedMeta> get_incomplete_meta();
	void put_meta(const SignedMeta& signed_meta, bool fully_assembled);

	bool put_allowed(const Meta::PathRevision& path_revision) noexcept;

	/* Index slots */
	void notify_all();  // Should be invoked after initialization of FolderGroup

	/* Chunk getter */
	uint32_t get_chunk_size(const blob& ct_hash);

	/* Properties */
	std::list<SignedMeta> containing_chunk(const blob& ct_hash);
	SQLiteDB& db() {return *db_;}

	status_t get_status();

private:
	FSFolder& dir_;
	Client& client_;

	std::unique_ptr<SQLiteDB> db_;	// Better use SOCI library ( https://github.com/SOCI/soci ). My "reinvented wheel" isn't stable enough.

	std::list<SignedMeta> get_meta(std::string sql, std::map<std::string, SQLValue> values = std::map<std::string, SQLValue>());
	void wipe();
};

} /* namespace librevault */
