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

#include "../../Directory.h"
#include "../../types.h"
#include "../../syncfs/Key.h"
#include <spdlog/logger.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <queue>

namespace librevault {
class Session;

namespace p2p {

class Announcer {
protected:
	using seconds = std::chrono::seconds;
	using time_point = std::chrono::time_point<std::chrono::steady_clock>;

	Session& session;
	std::shared_ptr<spdlog::logger> log;

	struct AnnouncedDirectory {
		std::shared_ptr<Directory> dir_ptr;
		unsigned int announced_times = 0;
		std::chrono::seconds interval;
		time_point last_announce;
	};
	std::map<std::shared_ptr<Directory>, std::shared_ptr<AnnouncedDirectory>> tracked_directories;

	struct time_comp {
		bool operator()(std::shared_ptr<AnnouncedDirectory> lhs, std::shared_ptr<AnnouncedDirectory> rhs){
			return (lhs->last_announce + lhs->interval) > (rhs->last_announce + rhs->interval);
		}
	};
	std::priority_queue<std::shared_ptr<AnnouncedDirectory>, std::deque<std::shared_ptr<AnnouncedDirectory>>, time_comp> announce_queue;
public:
	Announcer(Session& session);
	virtual ~Announcer();

	time_point get_queued_time() const;
	virtual seconds get_min_interval() const = 0;

	void add_directory(std::shared_ptr<Directory> directory);
	void remove_directory(std::shared_ptr<Directory> directory);
};

} /* namespace p2p */
} /* namespace librevault */
