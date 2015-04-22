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
#ifndef SRC_SYNCFS_SYNCMANAGER_H_
#define SRC_SYNCFS_SYNCMANAGER_H_

#include "Directory.h"
#include "../../contrib/dir_monitor/include/dir_monitor.hpp"

namespace librevault {
namespace sync {

class SyncManager {
	boost::asio::io_service& io_service;
	boost::asio::dir_monitor dir_monitor;

	std::set<std::shared_ptr<Directory>> directories;

public:
	SyncManager(boost::asio::io_service& io_service);
	virtual ~SyncManager();

	void add_directory(std::shared_ptr<Directory> directory);
};

} /* namespace sync */
} /* namespace librevault */

#endif /* SRC_SYNCFS_SYNCMANAGER_H_ */
