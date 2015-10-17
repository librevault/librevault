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
#include "ExchangeGroup.h"

#include "fs/FSDirectory.h"
#include "p2p/P2PDirectory.h"

namespace librevault {

ExchangeGroup::ExchangeGroup(Session& session, Exchanger& exchanger) :
		AbstractDirectory(session, exchanger) {}

std::vector<Meta::PathRevision> ExchangeGroup::get_meta_list() {
	return fs_dir_->get_meta_list();
}

void ExchangeGroup::broadcast_revision(std::shared_ptr<FSDirectory> origin, const Meta::PathRevision& revision) {
	std::lock_guard<std::mutex> lk(dirs_mtx_);
	for(auto& p2p_dir : p2p_dirs_) {
		p2p_dir->post_revision(origin, revision);
	}
}

void ExchangeGroup::post_revision(std::shared_ptr<AbstractDirectory> origin, const Meta::PathRevision& revision) {
	fs_dir_->post_revision(origin, revision);
}
void ExchangeGroup::request_meta(std::shared_ptr<AbstractDirectory> origin, const blob& path_id) {
	fs_dir_->request_meta(origin, path_id);
}
void ExchangeGroup::post_meta(std::shared_ptr<AbstractDirectory> origin, const Meta::SignedMeta& smeta) {
	fs_dir_->post_meta(origin, smeta);
}
void ExchangeGroup::request_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id) {
	fs_dir_->request_block(origin, block_id);
}
void ExchangeGroup::post_block(std::shared_ptr<AbstractDirectory> origin, const blob& encrypted_data_hash, const blob& block) {
	fs_dir_->post_block(origin, encrypted_data_hash, block);
}

void ExchangeGroup::attach_fs_dir(std::shared_ptr<FSDirectory> fs_dir_ptr) {
	fs_dir_ = fs_dir_ptr;
	fs_dir_->set_exchange_group(shared_from_this());
}

void ExchangeGroup::attach_p2p_dir(std::shared_ptr<P2PDirectory> remote_ptr) {
	std::lock_guard<std::mutex> lk(dirs_mtx_);

	for(auto& it : p2p_dirs_){
		if(it->remote_endpoint() == remote_ptr->remote_endpoint()) throw attach_error();
		if(it->remote_pubkey() == remote_ptr->remote_pubkey()) throw attach_error();
	}

	p2p_dirs_.insert(remote_ptr);
	log_->debug() << log_tag() << "Attached remote " << remote_ptr->remote_string();
}

void ExchangeGroup::detach_p2p_dir(std::shared_ptr<P2PDirectory> remote_ptr) {
	std::lock_guard<std::mutex> lk(dirs_mtx_);
	p2p_dirs_.erase(remote_ptr);
	log_->debug() << log_tag() << "Detached remote " << remote_ptr->remote_string();
}

bool ExchangeGroup::have_p2p_dir(const tcp_endpoint& endpoint) {
	std::lock_guard<std::mutex> lk(dirs_mtx_);
	for(auto& it : p2p_dirs_){
		if(it->remote_endpoint() == endpoint) return true;
	}
	return false;
}

bool ExchangeGroup::have_p2p_dir(const blob& pubkey) {
	std::lock_guard<std::mutex> lk(dirs_mtx_);
	for(auto& it : p2p_dirs_){
		if(it->remote_pubkey() == pubkey) return true;
	}
	return false;
}

std::set<std::shared_ptr<FSDirectory>> ExchangeGroup::fs_dirs() const {
	return std::set<std::shared_ptr<FSDirectory>>{fs_dir_};
}

const std::set<std::shared_ptr<P2PDirectory>>& ExchangeGroup::p2p_dirs() const {
	return p2p_dirs_;
}

const Key& ExchangeGroup::key() const {
	return fs_dir_->key();
}

const blob& ExchangeGroup::hash() const {
	return key().get_Hash();
}

std::string ExchangeGroup::name() const {
	if(name_.empty()) name_ = crypto::Base32().to_string(hash());
	return name_;
}

} /* namespace librevault */
