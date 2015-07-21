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
#include "../../pch.h"
#pragma once

#include "../Abstract.h"
#include "../../syncfs/SyncFS.h"

namespace librevault {

class FSProvider;
class FSDirectory : public AbstractDirectory {
	std::unique_ptr<syncfs::SyncFS> syncfs_ptr;
	ptree dir_options_;
	uint64_t received_bytes_ = 0;
	uint64_t sent_bytes_ = 0;
public:
	FSDirectory(ptree dir_options, Session& session, FSProvider& provider);
	~FSDirectory();

	std::string make_relative_path(const fs::path& path) const {return syncfs_ptr->make_portable_path(path);}

	void handle_modification(const boost::asio::dir_monitor_event& ev);

	const syncfs::Key& get_key() const {return syncfs_ptr->get_key();}
	fs::path get_open_path() const {return this->dir_options_.get<fs::path>("open_path");}
	//syncfs::SyncFS::Blocklist get_blocklist() {return syncfs_ptr->get_blocklist();}

	virtual blob get_hash() const {return get_key().get_Hash();}
};

} /* namespace librevault */
