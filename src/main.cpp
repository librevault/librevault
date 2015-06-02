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
#include "syncfs/SyncFS.h"
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>

int main(int argc, char** argv){
	boost::asio::io_service ios;

	// OpenBlockStorage part
	// SyncFS(boost::asio::io_service& io_service, Key key,	fs::path open_path,	fs::path block_path = open_path / ".librevault", fs::path db_path = open_path / ".librevault" / "directory.db");
	//librevault::syncfs::Key key;
	librevault::syncfs::Key key("ABQGUPXxCrtiSzH4QGo7Wb2szqRsieDJvR7NjYy3qSm8YJ");
	auto key_ro = key.derive(key.ReadOnly);
	auto key_down = key.derive(key.Download);
	BOOST_LOG_TRIVIAL(debug) << key.string() << " " << key_ro.string() << " " << key_down.string();
	librevault::syncfs::SyncFS syncfs_instance(ios, key_ro, "/home/gamepad/syncfs", "/home/gamepad/syncfs/.librevault", "/home/gamepad/syncfs/.librevault/directory.db");

	//syncfs_instance.index(syncfs_instance.get_openfs_file_list());

//	for(auto path : syncfs_instance.get_openfs_file_list()){
//		BOOST_LOG_TRIVIAL(debug) << path;
//		syncfs_instance.disassemble(path, false);
//	}
	syncfs_instance.assemble(false);
	ios.run();

	return 0;
}
