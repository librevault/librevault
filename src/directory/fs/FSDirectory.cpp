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
#include "FSDirectory.h"
#include "FSProvider.h"

#include "../../Session.h"
#include "../../net/parse_url.h"

namespace librevault {

FSDirectory::FSDirectory(ptree dir_options, Session& session, FSProvider& provider) :
		AbstractDirectory(session, provider),
		key_(dir_options.get<std::string>("key")),

		open_path_(dir_options.get<fs::path>("open_path")),
		block_path_(dir_options.get<fs::path>("block_path", open_path_ / ".librevault")),
		db_path_(dir_options.get<fs::path>("db_path", block_path_ / "directory.db")),
		asm_path_(dir_options.get<fs::path>("asm_path", block_path_ / "assembled.part")),

		monitor_(session.ios()) {
	log_->debug() << "New FSDirectory: Key type=" << (char)key_.get_type();

	index_ = std::make_unique<Index>(db_path_);
	enc_storage_ = std::make_unique<EncStorage>(block_path_);
	open_storage_ = std::make_unique<OpenStorage>(key_, *index_, *enc_storage_, open_path_, asm_path_);
	indexer_ = std::make_unique<Indexer>(key_, *index_, *enc_storage_, *open_storage_);

	indexer_->index(open_storage_->all_files());

	init_monitor(open_path_);
}

FSDirectory::~FSDirectory() {}

void FSDirectory::init_monitor(const fs::path& open_path) {
	if(key_.get_type() == key_.Download || key_.get_type() == key_.Owner){
		monitor_.add_directory(open_path.string());
		monitor_operation();
	}
}

void FSDirectory::monitor_operation() {
	monitor_.async_monitor([this](boost::system::error_code ec, boost::asio::dir_monitor_event ev){
		if(ec == boost::asio::error::operation_aborted) return;

		monitor_handle(ev);
		monitor_operation();
	});
}

void FSDirectory::monitor_handle(const boost::asio::dir_monitor_event& ev){
	log_->debug() << "[dir_monitor] " << ev;

	switch(ev.type){
	case boost::asio::dir_monitor_event::added:
	case boost::asio::dir_monitor_event::modified:
	case boost::asio::dir_monitor_event::renamed_old_name:
	case boost::asio::dir_monitor_event::renamed_new_name:
	case boost::asio::dir_monitor_event::removed:
		indexer_->index(open_storage_->make_relpath(ev.path));
	default: break;
	}
}

} /* namespace librevault */
