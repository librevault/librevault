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
#include "Downloader.h"

namespace librevault {

ExchangeGroup::ExchangeGroup(Client& client, Exchanger& exchanger) :
		Loggable(client), client_(client), exchanger_(exchanger), downloader_(std::make_shared<Downloader>(client, *this)) {}

void ExchangeGroup::request_introduce(std::shared_ptr<P2PDirectory> origin) {
	for(auto& info : fs_dir_->path_id_info()) {
		Meta::PathRevision revision;
		revision.path_id_ = info.first;
		revision.revision_ = info.second.first;
		origin->post_have_meta(revision, info.second.second);
	}
}

void ExchangeGroup::notify_meta(std::shared_ptr<FSDirectory> origin,
                                const Meta::PathRevision& revision,
                                const bitfield_type& bitfield) {
	downloader_->put_local_meta(revision, bitfield);

	// Broadcast to all attached P2PDirectories
	std::shared_lock<decltype(dirs_mtx_)> dirs_lk(dirs_mtx_);
	for(auto p2p_dir : p2p_dirs_) {
		p2p_dir->post_have_meta(revision, bitfield);
	}
}

void ExchangeGroup::notify_block(std::shared_ptr<FSDirectory> origin, const blob& encrypted_data_hash) {
	downloader_->remove_needed_block(encrypted_data_hash);

	std::shared_lock<decltype(dirs_mtx_)> dirs_lk(dirs_mtx_);

	for(auto p2p_dir : p2p_dirs_) {
		p2p_dir->post_have_block(encrypted_data_hash);
	}
}

void ExchangeGroup::notify_meta(std::shared_ptr<P2PDirectory> origin,
                                const Meta::PathRevision& revision,
                                const bitfield_type& bitfield) {
	if(!fs_dir_->have_meta(revision))
		origin->request_meta(revision);
}

void ExchangeGroup::handle_choke(std::shared_ptr<RemoteDirectory> origin) {
	downloader_->handle_choke(origin);
}
void ExchangeGroup::handle_unchoke(std::shared_ptr<RemoteDirectory> origin) {
	downloader_->handle_unchoke(origin);
}
void ExchangeGroup::handle_interested(std::shared_ptr<RemoteDirectory> origin) {
	// FIXME: write good algorithm for upload
	origin->unchoke();
}
void ExchangeGroup::handle_not_interested(std::shared_ptr<RemoteDirectory> origin) {
	// FIXME: write good algorithm for upload
	origin->choke();
}

void ExchangeGroup::request_meta(std::shared_ptr<RemoteDirectory> origin, const Meta::PathRevision& revision) {
	try {
		origin->post_meta(fs_dir_->get_meta(revision), fs_dir_->path_id_info().at(revision.path_id_).second);
	}catch(AbstractDirectory::no_such_meta& e){
		log_->warn() << log_tag() << "Requested nonexistent Meta";
	}
}

void ExchangeGroup::post_meta(std::shared_ptr<RemoteDirectory> origin, const Meta::SignedMeta& smeta) {
	origin->interest(); //TODO: remove this ugly hack
	fs_dir_->put_meta(smeta);
}

void ExchangeGroup::request_chunk(std::shared_ptr<RemoteDirectory> origin, const blob& encrypted_data_hash, uint32_t offset, uint32_t size) {
	auto chunk = fs_dir_->get_chunk(encrypted_data_hash, offset, size);
	origin->post_chunk(encrypted_data_hash, offset, chunk);
}

void ExchangeGroup::post_chunk(std::shared_ptr<RemoteDirectory> origin, const blob& encrypted_data_hash, const blob& chunk, uint32_t offset) {
	downloader_->put_chunk(encrypted_data_hash, offset, chunk, origin);
}

void ExchangeGroup::attach(std::shared_ptr<FSDirectory> fs_dir_ptr) {
	std::unique_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	fs_dir_ = fs_dir_ptr;
	fs_dir_->exchange_group_ = shared_from_this();
	name_ = crypto::Base32().to_string(hash());

	log_->debug() << log_tag() << "Attached local " << fs_dir_ptr->name();
}

void ExchangeGroup::attach(std::shared_ptr<P2PDirectory> remote_ptr) {
	if(have_p2p_dir(remote_ptr->remote_endpoint()) || have_p2p_dir(remote_ptr->remote_pubkey())) throw attach_error();

	std::unique_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	remote_ptr->exchange_group_ = shared_from_this();

	p2p_dirs_.insert(remote_ptr);
	p2p_dirs_endpoints_.insert(remote_ptr->remote_endpoint());
	p2p_dirs_pubkeys_.insert(remote_ptr->remote_pubkey());

	log_->debug() << log_tag() << "Attached remote " << remote_ptr->name();
}

void ExchangeGroup::detach(std::shared_ptr<P2PDirectory> remote_ptr) {
	std::unique_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	p2p_dirs_pubkeys_.erase(remote_ptr->remote_pubkey());
	p2p_dirs_endpoints_.erase(remote_ptr->remote_endpoint());
	p2p_dirs_.erase(remote_ptr);

	log_->debug() << log_tag() << "Detached remote " << remote_ptr->name();
}

bool ExchangeGroup::have_p2p_dir(const tcp_endpoint& endpoint) {
	std::shared_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	return p2p_dirs_endpoints_.find(endpoint) != p2p_dirs_endpoints_.end();
}

bool ExchangeGroup::have_p2p_dir(const blob& pubkey) {
	std::shared_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	return p2p_dirs_pubkeys_.find(pubkey) != p2p_dirs_pubkeys_.end();
}

std::set<std::shared_ptr<FSDirectory>> ExchangeGroup::fs_dirs() const {
	std::shared_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	return std::set<std::shared_ptr<FSDirectory>>{fs_dir_};
}

const std::set<std::shared_ptr<P2PDirectory>>& ExchangeGroup::p2p_dirs() const { return p2p_dirs_; }

const Key& ExchangeGroup::key() const { return fs_dir_->key(); }
const blob& ExchangeGroup::hash() const { return key().get_Hash(); }

} /* namespace librevault */
