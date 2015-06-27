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
#include "Announcer.h"

namespace librevault {
namespace p2p {

Announcer::Announcer(io_service& ios, Signals& signals, ptree& options) : ios(ios), signals(signals), options(options) {}
Announcer::~Announcer() {}

void Announcer::start_announce_timer(){
	if(announce_queue.empty()) return;

	announce_timer.expires_at(announce_queue.top()->last_announce+std::min(announce_queue.top()->interval, std::chrono::seconds(options.get<int>("discovery.bttracker.min_interval"))));
	announce_timer.async_wait([this](const boost::system::error_code& ec){
		if(ec == boost::asio::error::operation_aborted) return;
		announce(announce_queue.top()->dir_ptr);
		announce_queue.pop();
	});
}

void Announcer::stop_announce_timer(){
	announce_timer.cancel();
}

void Announcer::add_directory(FSDirectory* directory){
	auto new_announced_directory = std::make_shared<AnnouncedDirectory>();
	new_announced_directory->dir_ptr = directory;
	new_announced_directory->interval = std::chrono::seconds(options.get<int>("discovery.bttracker.min_interval"));

	tracked_directories.insert(std::make_pair(directory, new_announced_directory));
	start_announce_timer();
}

void Announcer::remove_directory(FSDirectory* directory){
	start_announce_timer();
}

} /* namespace p2p */
} /* namespace librevault */
