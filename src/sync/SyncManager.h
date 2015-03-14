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
#ifndef SRC_SYNC_SYNCMANAGER_H_
#define SRC_SYNC_SYNCMANAGER_H_

#include <boost/filesystem.hpp>
#include <set>

namespace librevault {
class Directory;

class SyncManager {
	std::set<Directory*> directories;
	boost::filesystem::path configdir;
public:
	SyncManager();
	virtual ~SyncManager();

	void addDirectory(Directory* directory);
};

} /* namespace librevault */

#endif /* SRC_SYNC_SYNCMANAGER_H_ */
