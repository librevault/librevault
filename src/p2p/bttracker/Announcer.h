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

#include "../../types.h"
#include "../../syncfs/Key.h"
#include "../../Signals.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <queue>

namespace librevault {
namespace p2p {

using boost::property_tree::ptree;

class Announcer {
protected:
	io_service& ios;
	Signals& signals;
	ptree& options;

	struct AnnouncedDirectory {
		FSDirectory* dir_ptr;
		unsigned int announced_times = 0;
		std::chrono::seconds interval;
		std::chrono::time_point<std::chrono::steady_clock> last_announce;
	};
	std::map<FSDirectory*, std::shared_ptr<AnnouncedDirectory>> tracked_directories;

	struct time_comp {
		bool operator()(std::shared_ptr<AnnouncedDirectory> lhs, std::shared_ptr<AnnouncedDirectory> rhs){
			return (lhs->last_announce + lhs->interval) > (rhs->last_announce + rhs->interval);
		}
	};
	std::priority_queue<std::shared_ptr<AnnouncedDirectory>, std::deque<std::shared_ptr<AnnouncedDirectory>>, time_comp> announce_queue;

	boost::asio::steady_timer announce_timer;
	void start_announce_timer();
	void stop_announce_timer();

	virtual void announce(FSDirectory* directory) = 0;
public:
	Announcer(io_service& ios, Signals& signals, ptree& options);
	virtual ~Announcer();

	virtual void start() = 0;

	void add_directory(FSDirectory* directory);
	void remove_directory(FSDirectory* directory);
};

} /* namespace p2p */
} /* namespace librevault */
