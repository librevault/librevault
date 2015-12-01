/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ExchangeGroup.h"

#include "fs/FSDirectory.h"
#include "p2p/P2PDirectory.h"

#include "p2p/P2PProvider.h"

#include "Exchanger.h"
#include "../Client.h"

namespace librevault {

ExchangeGroup::ExchangeGroup(Client& client, Exchanger& exchanger) :
		Loggable(client), client_(client), exchanger_(exchanger) {}

void ExchangeGroup::post_have_meta(std::shared_ptr<FSDirectory> origin, const Meta::PathRevision& revision, const AbstractDirectory::bitfield_type& bitfield) {
	// Broadcast to all attached P2PDirectories
	std::shared_lock<std::shared_timed_mutex> dirs_lk(dirs_mtx_);
	for(auto p2p_dir : p2p_dirs_) {
		p2p_dir->post_have_meta(revision, bitfield);
	}
}

void ExchangeGroup::post_have_block(std::shared_ptr<FSDirectory> origin, const blob& encrypted_data_hash) {
	std::shared_lock<std::shared_timed_mutex> dirs_lk(dirs_mtx_);
	for(auto p2p_dir : p2p_dirs_) {
		p2p_dir->post_have_block(encrypted_data_hash);
	}
}

void ExchangeGroup::attach(std::shared_ptr<FSDirectory> fs_dir_ptr) {
	std::unique_lock<std::shared_timed_mutex> lk(dirs_mtx_);
	fs_dir_ = fs_dir_ptr;
	fs_dir_->exchange_group_ = shared_from_this();
	name_ = crypto::Base32().to_string(hash());

	log_->debug() << log_tag() << "Attached local " << fs_dir_ptr->name();
}

void ExchangeGroup::attach(std::shared_ptr<P2PDirectory> remote_ptr) {
	if(have_p2p_dir(remote_ptr->remote_endpoint()) || have_p2p_dir(remote_ptr->remote_pubkey())) throw attach_error();

	std::unique_lock<std::shared_timed_mutex> lk(dirs_mtx_);
	remote_ptr->exchange_group_ = shared_from_this();
	exchanger_.p2p_provider()->remove_from_unattached(remote_ptr);

	p2p_dirs_.insert(remote_ptr);
	p2p_dirs_endpoints_.insert(remote_ptr->remote_endpoint());
	p2p_dirs_pubkeys_.insert(remote_ptr->remote_pubkey());

	log_->debug() << log_tag() << "Attached remote " << remote_ptr->name();
}

void ExchangeGroup::detach(std::shared_ptr<P2PDirectory> remote_ptr) {
	std::unique_lock<std::shared_timed_mutex> lk(dirs_mtx_);
	p2p_dirs_pubkeys_.erase(remote_ptr->remote_pubkey());
	p2p_dirs_endpoints_.erase(remote_ptr->remote_endpoint());
	p2p_dirs_.erase(remote_ptr);

	log_->debug() << log_tag() << "Detached remote " << remote_ptr->name();
}

bool ExchangeGroup::have_p2p_dir(const tcp_endpoint& endpoint) {
	std::shared_lock<std::shared_timed_mutex> lk(dirs_mtx_);
	return p2p_dirs_endpoints_.find(endpoint) != p2p_dirs_endpoints_.end();
}

bool ExchangeGroup::have_p2p_dir(const blob& pubkey) {
	std::shared_lock<std::shared_timed_mutex> lk(dirs_mtx_);
	return p2p_dirs_pubkeys_.find(pubkey) != p2p_dirs_pubkeys_.end();
}

std::set<std::shared_ptr<FSDirectory>> ExchangeGroup::fs_dirs() const {
	std::shared_lock<std::shared_timed_mutex> lk(dirs_mtx_);
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

} /* namespace librevault */
