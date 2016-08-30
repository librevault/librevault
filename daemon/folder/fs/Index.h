/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
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
