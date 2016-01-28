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
#include "../pch.h"
#pragma once
#include "../util/Loggable.h"
#include <librevault/Secret.h>
#include "AbstractFolder.h"

namespace librevault {

class Client;

class RemoteFolder;
class FSFolder;
class P2PFolder;

class Uploader;
class Downloader;

class FolderGroup : public std::enable_shared_from_this<FolderGroup>, protected Loggable {
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("FolderGroup error") {}
	};

	struct attach_error : error {
		attach_error() : error("Could not attach remote to FolderGroup") {}
	};

	FolderGroup(Client& client);

	/* Actions */
	// FSFolder actions
	void notify_meta(std::shared_ptr<FSFolder> origin, Meta::PathRevision revision, bitfield_type bitfield);
	void notify_block(std::shared_ptr<FSFolder> origin, const blob& encrypted_data_hash);

	// RemoteFolder actions
	void handle_handshake(std::shared_ptr<RemoteFolder> origin);

	void handle_choke(std::shared_ptr<RemoteFolder> origin);
	void handle_unchoke(std::shared_ptr<RemoteFolder> origin);
	void handle_interested(std::shared_ptr<RemoteFolder> origin);
	void handle_not_interested(std::shared_ptr<RemoteFolder> origin);

	void notify_meta(std::shared_ptr<RemoteFolder> origin, const Meta::PathRevision& revision, const bitfield_type& bitfield);
	void notify_block(std::shared_ptr<RemoteFolder> origin, const blob& encrypted_data_hash);

	void request_meta(std::shared_ptr<RemoteFolder> origin, const Meta::PathRevision& revision);
	void post_meta(std::shared_ptr<RemoteFolder> origin, const Meta::SignedMeta& smeta, const bitfield_type& bitfield);

	void request_chunk(std::shared_ptr<RemoteFolder> origin, const blob& encrypted_data_hash, uint32_t offset, uint32_t size);
	void post_chunk(std::shared_ptr<RemoteFolder> origin, const blob& encrypted_data_hash, const blob& chunk, uint32_t offset);

	/* Membership management */
	void attach(std::shared_ptr<FSFolder> fs_dir_ptr);
	void attach(std::shared_ptr<P2PFolder> remote_ptr);
	void detach(std::shared_ptr<P2PFolder> remote_ptr);

	bool have_p2p_dir(const tcp_endpoint& endpoint);
	bool have_p2p_dir(const blob& pubkey);

	/* Getters */
	std::shared_ptr<FSFolder> fs_dir() const {return fs_dir_;}
	std::set<std::shared_ptr<P2PFolder>> p2p_dirs() const {return p2p_dirs_;}

	const Secret& secret() const;
	const blob& hash() const;
private:
	Client& client_;

	std::shared_ptr<Uploader> uploader_;
	std::shared_ptr<Downloader> downloader_;

	/* Members */
	mutable std::shared_timed_mutex dirs_mtx_;

	std::shared_ptr<FSFolder> fs_dir_;
	std::set<std::shared_ptr<P2PFolder>> p2p_dirs_;

	// Member lookup optimization
	std::set<blob> p2p_dirs_pubkeys_;
	std::set<tcp_endpoint> p2p_dirs_endpoints_;

	/* Loggable override */
	std::string name() const {return name_;}
};

} /* namespace librevault */
