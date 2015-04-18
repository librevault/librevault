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
#ifndef SRC_SYNCFS_SYNCFS_H_
#define SRC_SYNCFS_SYNCFS_H_

#include <boost/filesystem.hpp>
#include <vector>

namespace librevault {
namespace syncfs {

namespace fs = boost::filesystem;
using blob = std::vector<uint8_t>;

} /* namespace syncfs */
} /* namespace librevault */

#endif /* SRC_SYNCFS_SYNCFS_H_ */
