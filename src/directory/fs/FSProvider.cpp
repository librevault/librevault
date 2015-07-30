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

FSProvider::FSProvider(Session& session, DirectoryExchanger& exchanger) : AbstractProvider(session, exchanger) {
	auto folder_trees = session.config().equal_range("folder");
	for(auto folder_tree_it = folder_trees.first; folder_tree_it != folder_trees.second; folder_tree_it++){
		add_directory(folder_tree_it->second);
	}
}
FSProvider::~FSProvider() {}

void FSProvider::register_directory(std::shared_ptr<FSDirectory> dir_ptr) {
	log_->info() << "Registering directory: k=" << dir_ptr->key() << " path=" << dir_ptr->open_path();

	hash_dir_.insert({dir_ptr->key().get_Hash(), dir_ptr});
	path_dir_.insert({dir_ptr->open_path(), dir_ptr});

	log_->info() << "Registered directory: k=" << dir_ptr->key() << " path=" << dir_ptr->open_path();
}

void FSProvider::unregister_directory(std::shared_ptr<FSDirectory> dir_ptr) {
	log_->info() << "Unregistering directory: k=" << dir_ptr->key() << " path=" << dir_ptr->open_path();

	path_dir_.erase(dir_ptr->open_path());
	hash_dir_.erase(dir_ptr->key().get_Hash());

	log_->info() << "Unregistered directory: k=" << dir_ptr->key() << " path=" << dir_ptr->open_path();
}

void FSProvider::add_directory(ptree dir_options) {
	auto dir_ptr = std::make_shared<FSDirectory>(dir_options, session_, *this);
	register_directory(dir_ptr);
}

} /* namespace librevault */
