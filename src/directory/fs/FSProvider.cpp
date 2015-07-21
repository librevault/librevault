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
#include "FSProvider.h"
#include "FSDirectory.h"

#include "../../Session.h"

namespace librevault {

FSProvider::FSProvider(Session& session, DirectoryExchanger& exchanger) : AbstractProvider(session, exchanger), monitor_(session.ios()) {
	auto folder_trees = session.config().equal_range("folder");
	for(auto folder_tree_it = folder_trees.first; folder_tree_it != folder_trees.second; folder_tree_it++){
		add_directory(folder_tree_it->second);
	}
	monitor_operation();
}
FSProvider::~FSProvider() {}

void FSProvider::monitor_operation() {
	monitor_.async_monitor([this](boost::system::error_code ec, boost::asio::dir_monitor_event ev){
		if(ec == boost::asio::error::operation_aborted) return;
		log_->debug() << "[dir_monitor] " << ev;
		fs::path ev_path = fs::absolute(ev.path);
		for(auto it : path_dir_){
			auto path_dir_it = it.first.begin();
			auto ev_dir_it = ev_path.begin();
			bool equal = true;
			while(path_dir_it != it.first.end()){
				if(*path_dir_it != *ev_dir_it)	equal = false;
				path_dir_it++; ev_dir_it++;
			}
			if(equal)
				it.second->handle_modification(ev);
		}

		monitor_operation();
	});
}

void FSProvider::register_directory(std::shared_ptr<FSDirectory> dir_ptr) {
	log_->info() << "Registering directory: k=" << dir_ptr->get_key() << " path=" << dir_ptr->get_open_path();

	hash_dir_.insert({dir_ptr->get_key().get_Hash(), dir_ptr});
	path_dir_.insert({dir_ptr->get_open_path(), dir_ptr});

	if(dir_ptr->get_key().get_type() != syncfs::Key::Download)
		monitor_.add_directory(dir_ptr->get_open_path().string());

	log_->info() << "Registered directory: k=" << dir_ptr->get_key() << " path=" << dir_ptr->get_open_path();
}

void FSProvider::unregister_directory(std::shared_ptr<FSDirectory> dir_ptr) {
	log_->info() << "Unregistering directory: k=" << dir_ptr->get_key() << " path=" << dir_ptr->get_open_path();

	if(dir_ptr->get_key().get_type() != syncfs::Key::Download)
		monitor_.remove_directory(dir_ptr->get_open_path().string());

	path_dir_.erase(dir_ptr->get_open_path());
	hash_dir_.erase(dir_ptr->get_key().get_Hash());

	log_->info() << "Unregistered directory: k=" << dir_ptr->get_key() << " path=" << dir_ptr->get_open_path();
}

void FSProvider::add_directory(ptree dir_options) {
	auto dir_ptr = std::make_shared<FSDirectory>(dir_options, session_, *this);
	register_directory(dir_ptr);
	unregister_directory(dir_ptr);
}

} /* namespace librevault */
