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

#include <memory>
#include <boost/signals2.hpp>

namespace librevault {

class FSDirectory;
struct Signals {
	enum SignalType {
		UNKNOWN = 0,
		DIRECTORY_ADDED,
		DIRECTORY_REMOVED,
		DIRECTORY_CHANGED
	};

	boost::signals2::signal<void(std::shared_ptr<FSDirectory>, SignalType)> directory;
};

} /* namespace librevault */
