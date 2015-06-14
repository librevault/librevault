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
#include "Directory.h"

namespace librevault {

Directory::Directory(io_service& ios, boost::asio::dir_monitor& monitor, ptree dir_options, ptree& options) :
		ios(ios), monitor(monitor), dir_options(std::move(dir_options)), options(options) {
	key = this->dir_options.get<std::string>("key");

	fs::path open_path = this->dir_options.get<fs::path>("open_path");
	fs::path block_path = this->dir_options.get<fs::path>("block_path", open_path / ".librevault");
	fs::path db_path = this->dir_options.get<fs::path>("db_path", open_path / ".librevault" / "directory.db");
	syncfs_ptr = std::make_unique<syncfs::SyncFS>(ios, key, open_path, block_path, db_path);

	monitor.add_directory(open_path.string());
}
Directory::~Directory() {
	monitor.remove_directory(dir_options.get<fs::path>("open_path").string());
}

void Directory::handle_modification(const boost::asio::dir_monitor_event& ev){
	switch(ev.type){
	case ev.renamed_old_name:
	case ev.added:
		break;

	default:
		syncfs_ptr->index(make_relative_path(ev.path));
	}
}

} /* namespace librevault */
