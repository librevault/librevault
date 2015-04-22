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
#include "SyncManager.h"

namespace librevault {
namespace sync {

SyncManager::SyncManager(boost::asio::io_service& io_service) : io_service(io_service), dir_monitor(io_service) {}
SyncManager::~SyncManager() {}

void SyncManager::add_directory(std::shared_ptr<Directory> directory) {
	/*dir_monitor.add_directory(argv[1]);
	dir_monitor.async_monitor([&](const boost::system::error_code& ec, boost::asio::dir_monitor_event ev){
				if(ev.type == boost::asio::dir_monitor_event::added){
					cryptodiff::FileMap map(key);
					std::ifstream istream(ev.path.c_str(), std::ios_base::in | std::ios_base::binary);
					map.create(istream);
				}

				std::cout << ev << std::endl;
			});*/
}

} /* namespace sync */
} /* namespace librevault */
