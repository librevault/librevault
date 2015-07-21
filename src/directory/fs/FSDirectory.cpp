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
#include "FSDirectory.h"
#include "FSProvider.h"

#include "../../Session.h"
#include "../../net/parse_url.h"

namespace librevault {

FSDirectory::FSDirectory(ptree dir_options, Session& session, FSProvider& provider) :
		AbstractDirectory(session, provider),
		dir_options_(std::move(dir_options)) {
	syncfs::Key key = this->dir_options_.get<std::string>("key");

	fs::path open_path = get_open_path();
	fs::path block_path = this->dir_options_.get<fs::path>("block_path", open_path / ".librevault");
	fs::path db_path = this->dir_options_.get<fs::path>("db_path", open_path / ".librevault" / "directory.db");
	syncfs_ptr = std::make_unique<syncfs::SyncFS>(session.ios(), key, open_path, block_path, db_path);

	// Attaching nodes
	auto node_iterators = this->dir_options_.equal_range("node");
	std::set<tcp_endpoint> nodes_set;
	for(auto it = node_iterators.first; it != node_iterators.second; it++){
		url node_url = parse_url(it->second.get_value<std::string>());
		nodes_set.insert(tcp_endpoint(address::from_string(node_url.host), node_url.port));	//TODO: Add support for hostnames
	}
}

FSDirectory::~FSDirectory() {}

void FSDirectory::handle_modification(const boost::asio::dir_monitor_event& ev){
	switch(ev.type){
	case boost::asio::dir_monitor_event::renamed_old_name:
	case boost::asio::dir_monitor_event::added:
		break;

	default:
		syncfs_ptr->index(make_relative_path(ev.path));
	}
}

} /* namespace librevault */
