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
#include "FolderGroup.h"

#include "src/folder/fs/FSFolder.h"
#include "src/folder/p2p/P2PFolder.h"

#include "fs/Index.h"

#include "../Client.h"

#include "Uploader.h"
#include "Downloader.h"

namespace librevault {

FolderGroup::FolderGroup(Client& client) :
		Loggable(client),
		client_(client),
		uploader_(std::make_shared<Uploader>(client, *this)),
		downloader_(std::make_shared<Downloader>(client, *this)) {}

/* Actions */
// FSFolder actions
void FolderGroup::notify_meta(std::shared_ptr<FSFolder> origin,
                              Meta::PathRevision revision,
                              bitfield_type bitfield) {
	downloader_->notify_local_meta(revision, bitfield);

	// Broadcast to all attached P2PDirectories
	std::unique_lock<decltype(dirs_mtx_)> dirs_lk(dirs_mtx_);
	for(auto p2p_dir : p2p_dirs_) {
		p2p_dir->post_have_meta(revision, bitfield);
	}
}

void FolderGroup::notify_chunk(std::shared_ptr<FSFolder> origin, const blob& ct_hash) {
	downloader_->notify_local_chunk(ct_hash);

	std::unique_lock<decltype(dirs_mtx_)> dirs_lk(dirs_mtx_);

	for(auto p2p_dir : p2p_dirs_) {
		p2p_dir->post_have_chunk(ct_hash);
	}
}

// RemoteFolder actions
void FolderGroup::handle_handshake(std::shared_ptr<RemoteFolder> origin) {
	for(auto& meta : fs_dir_->index->get_Meta()) {
		origin->post_have_meta(meta.meta().path_revision(), fs_dir_->get_bitfield(meta.meta().path_revision()));
	}
}

void FolderGroup::handle_choke(std::shared_ptr<RemoteFolder> origin) {
	downloader_->handle_choke(origin);
}
void FolderGroup::handle_unchoke(std::shared_ptr<RemoteFolder> origin) {
	downloader_->handle_unchoke(origin);
}
void FolderGroup::handle_interested(std::shared_ptr<RemoteFolder> origin) {
	uploader_->handle_interested(origin);
}
void FolderGroup::handle_not_interested(std::shared_ptr<RemoteFolder> origin) {
	uploader_->handle_not_interested(origin);
}

void FolderGroup::notify_meta(std::shared_ptr<RemoteFolder> origin,
                              const Meta::PathRevision& revision,
                              const bitfield_type& bitfield) {
	if(fs_dir_->have_meta(revision))
		downloader_->notify_remote_meta(origin, revision, bitfield);
	else
		origin->request_meta(revision);
}

void FolderGroup::notify_chunk(std::shared_ptr<RemoteFolder> origin, const blob& ct_hash) {
	downloader_->notify_remote_chunk(origin, ct_hash);
}

void FolderGroup::request_meta(std::shared_ptr<RemoteFolder> origin, const Meta::PathRevision& revision) {
	try {
		origin->post_meta(fs_dir_->get_meta(revision), fs_dir_->get_bitfield(revision));
	}catch(AbstractFolder::no_such_meta& e){
		log_->warn() << log_tag() << "Requested nonexistent Meta";
	}
}

void FolderGroup::post_meta(std::shared_ptr<RemoteFolder> origin, const SignedMeta& smeta, const bitfield_type& bitfield) {
	fs_dir_->put_meta(smeta);
	downloader_->notify_remote_meta(origin, smeta.meta().path_revision(), bitfield);
}

void FolderGroup::request_block(std::shared_ptr<RemoteFolder> origin, const blob& ct_hash, uint32_t offset, uint32_t size) {
	uploader_->request_block(origin, ct_hash, offset, size);
}

void FolderGroup::post_chunk(std::shared_ptr<RemoteFolder> origin, const blob& ct_hash, const blob& chunk, uint32_t offset) {
	downloader_->put_block(ct_hash, offset, chunk, origin);
}

/* Membership management */
void FolderGroup::attach(std::shared_ptr<FSFolder> fs_dir_ptr) {
	std::unique_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	fs_dir_ = fs_dir_ptr;
	fs_dir_->folder_group_ = shared_from_this();
	name_ = crypto::Base32().to_string(hash());

	log_->debug() << log_tag() << "Attached local " << fs_dir_ptr->name();
}

void FolderGroup::attach(std::shared_ptr<P2PFolder> remote_ptr) {
	if(have_p2p_dir(remote_ptr->remote_endpoint()) || have_p2p_dir(remote_ptr->remote_pubkey())) throw attach_error();

	std::unique_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	remote_ptr->folder_group_ = shared_from_this();

	p2p_dirs_.insert(remote_ptr);
	p2p_dirs_endpoints_.insert(remote_ptr->remote_endpoint());
	p2p_dirs_pubkeys_.insert(remote_ptr->remote_pubkey());

	log_->debug() << log_tag() << "Attached remote " << remote_ptr->name();
}

void FolderGroup::detach(std::shared_ptr<P2PFolder> remote_ptr) {
	std::unique_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	downloader_->erase_remote(remote_ptr);

	p2p_dirs_pubkeys_.erase(remote_ptr->remote_pubkey());
	p2p_dirs_endpoints_.erase(remote_ptr->remote_endpoint());
	p2p_dirs_.erase(remote_ptr);

	log_->debug() << log_tag() << "Detached remote " << remote_ptr->name();
}

bool FolderGroup::have_p2p_dir(const tcp_endpoint& endpoint) {
	std::unique_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	return p2p_dirs_endpoints_.find(endpoint) != p2p_dirs_endpoints_.end();
}

bool FolderGroup::have_p2p_dir(const blob& pubkey) {
	std::unique_lock<decltype(dirs_mtx_)> lk(dirs_mtx_);
	return p2p_dirs_pubkeys_.find(pubkey) != p2p_dirs_pubkeys_.end();
}

/* Getters */
const Secret& FolderGroup::secret() const { return fs_dir_->secret(); }
const blob& FolderGroup::hash() const { return secret().get_Hash(); }

} /* namespace librevault */
