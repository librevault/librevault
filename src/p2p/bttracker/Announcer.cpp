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
#include "../../Session.h"

namespace librevault {
namespace p2p {

Announcer::Announcer(Session& session) : session(session) {}
Announcer::~Announcer() {}

Announcer::time_point Announcer::get_queued_time() const {
	return announce_queue.top()->last_announce+std::max(announce_queue.top()->interval, get_min_interval());
}

void Announcer::add_directory(std::shared_ptr<Directory> directory){
	log->debug() << "Adding directory: k=" << directory->get_key();

	auto new_announced_directory = std::make_shared<AnnouncedDirectory>();
	new_announced_directory->dir_ptr = directory;
	new_announced_directory->interval = get_min_interval();

	tracked_directories.insert(std::make_pair(directory, new_announced_directory));

	log->info() << "Added directory: k=" << directory->get_key();
}

void Announcer::remove_directory(std::shared_ptr<Directory> directory){
	log->debug() << "Removing directory: k=" << directory->get_key();

	auto it = tracked_directories.find(directory);
	if(it != tracked_directories.end()){
		it->second->dir_ptr.reset();
		tracked_directories.erase(it);
	}

	log->info() << "Removed directory: k=" << directory->get_key();
}

} /* namespace p2p */
} /* namespace librevault */
