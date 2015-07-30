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
#include "../Abstract.h"

#include "../Key.h"
#include "Index.h"
#include "EncStorage.h"
#include "OpenStorage.h"
#include "Indexer.h"

namespace librevault {

class FSProvider;
class FSDirectory : public AbstractDirectory {
public:
	FSDirectory(ptree dir_options, Session& session, FSProvider& provider);
	~FSDirectory();

	const Key& key() const {return key_;}
	virtual blob hash() const {return key().get_Hash();}

	const fs::path& open_path() const {return open_path_;}
	const fs::path& block_path() const {return block_path_;}
	const fs::path& db_path() const {return db_path_;}
	const fs::path& asm_path() const {return asm_path_;}

private:
	// Key
	const Key key_;

	// Paths
	const fs::path open_path_, block_path_, db_path_, asm_path_;

	// Storages
	std::unique_ptr<Index> index_;
	std::unique_ptr<EncStorage> enc_storage_;
	std::unique_ptr<OpenStorage> open_storage_;
	std::unique_ptr<Indexer> indexer_;

	// Monitor
	boost::asio::dir_monitor monitor_;

	// Statistics
	uint64_t received_bytes_ = 0;
	uint64_t sent_bytes_ = 0;

	// Initialization
	void init_monitor(const fs::path& open_path);

	// Monitor operations
	void monitor_operation();
	void monitor_handle(const boost::asio::dir_monitor_event& ev);
};

} /* namespace librevault */
