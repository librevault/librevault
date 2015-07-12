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
#include "Directory.h"
#include "Session.h"

namespace librevault {

Directory::Directory(ptree dir_options, Session& session) :
		dir_options(std::move(dir_options)), session(session) {
	syncfs::Key key = this->dir_options.get<std::string>("key");
	auto smallkey = key.string(); smallkey.resize(7);

	log = spdlog::stderr_logger_mt(std::string("dir(")+smallkey+")");

	fs::path open_path = get_open_path();
	fs::path block_path = this->dir_options.get<fs::path>("block_path", open_path / ".librevault");
	fs::path db_path = this->dir_options.get<fs::path>("db_path", open_path / ".librevault" / "directory.db");
	syncfs_ptr = std::make_unique<syncfs::SyncFS>(session.get_ios(), key, open_path, block_path, db_path);

	session.get_monitor().add_directory(open_path.string());

	auto node_iterators = this->dir_options.equal_range("node");
	std::set<tcp_endpoint> nodes_set;
	for(auto it = node_iterators.first; it != node_iterators.second; it++){
		url node_url = parse_url(it->second.get_value<std::string>());
		nodes_set.insert(tcp_endpoint(address::from_string(node_url.host), node_url.port));
	}
	add_node(nodes_set);
}

Directory::~Directory() {
	session.get_monitor().remove_directory(get_open_path().string());
}

void Directory::add_node(const std::set<tcp_endpoint>& nodes){
	for(auto endpoint : nodes){
		log->info() << "Added node: " << endpoint;
	}
}

void Directory::handle_modification(const boost::asio::dir_monitor_event& ev){
	switch(ev.type){
	case boost::asio::dir_monitor_event::renamed_old_name:
	case boost::asio::dir_monitor_event::added:
		break;

	default:
		syncfs_ptr->index(make_relative_path(ev.path));
	}
}

} /* namespace librevault */
