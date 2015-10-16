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
#include "Exchanger.h"

#include "fs/FSDirectory.h"
#include "p2p/P2PProvider.h"
#include "../discovery/MulticastDiscovery.h"

namespace librevault {

Exchanger::Exchanger(Session& session) : session_(session), log_(spdlog::get("Librevault")) {
	auto folder_trees = session.config().equal_range("folder");
	for(auto folder_tree_it = folder_trees.first; folder_tree_it != folder_trees.second; folder_tree_it++){
		add_directory(folder_tree_it->second);
	}

	p2p_provider_ = std::make_unique<P2PProvider>(session, *this);

	multicast4_ = std::make_unique<MulticastDiscovery4>(p2p_provider_.get(), session_, *this);
	multicast6_ = std::make_unique<MulticastDiscovery6>(p2p_provider_.get(), session_, *this);
}
Exchanger::~Exchanger() {}

std::shared_ptr<FSDirectory> Exchanger::get_directory(const fs::path& path){
	auto it = path_dir_.find(path);
	if(it != path_dir_.end())
		return it->second;
	return nullptr;
}

std::shared_ptr<FSDirectory> Exchanger::get_directory(const blob& hash){
	auto it = hash_dir_.find(hash);
	if(it != hash_dir_.end())
		return it->second;
	return nullptr;
}

void Exchanger::register_directory(std::shared_ptr<FSDirectory> dir_ptr) {
	path_dir_.insert({dir_ptr->open_path(), dir_ptr});
	hash_dir_.insert({dir_ptr->key().get_Hash(), dir_ptr});

	// Did this to defer execution to moment, when multicast4_,multicast6_ is initialized
	session_.ios().post(std::bind([this](std::shared_ptr<FSDirectory> dir_ptr){multicast4_->register_directory(dir_ptr);}, dir_ptr));
	session_.ios().post(std::bind([this](std::shared_ptr<FSDirectory> dir_ptr){multicast6_->register_directory(dir_ptr);}, dir_ptr));
}

void Exchanger::unregister_directory(std::shared_ptr<FSDirectory> dir_ptr) {
	multicast4_->unregister_directory(dir_ptr);
	multicast6_->unregister_directory(dir_ptr);

	hash_dir_.erase(dir_ptr->key().get_Hash());
	path_dir_.erase(dir_ptr->open_path());
}

void Exchanger::add_directory(const ptree& dir_options) {
	auto dir_ptr = std::make_shared<FSDirectory>(dir_options, session_, *this);
	register_directory(dir_ptr);
}

} /* namespace librevault */
