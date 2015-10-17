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
#include "../pch.h"
#pragma once
#include "../util/Loggable.h"
#include "Key.h"
#include "Abstract.h"

namespace librevault {

class Session;
class Exchanger;
class FSDirectory;
class P2PDirectory;

class ExchangeGroup : public std::enable_shared_from_this<ExchangeGroup>, public AbstractDirectory {
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("ExchangeGroup error") {}
	};

	struct attach_error : error {
		attach_error() : error("Could not attach remote to FSDirectory") {}
	};

	ExchangeGroup(Session& session, Exchanger& exchanger);

	std::vector<Meta::PathRevision> get_meta_list();

	void broadcast_revision(std::shared_ptr<FSDirectory> origin, const Meta::PathRevision& revision);

	void post_revision(std::shared_ptr<AbstractDirectory> origin, const Meta::PathRevision& revision);
	void request_meta(std::shared_ptr<AbstractDirectory> origin, const blob& path_id);
	void post_meta(std::shared_ptr<AbstractDirectory> origin, const Meta::SignedMeta& smeta);
	void request_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id);
	void post_block(std::shared_ptr<AbstractDirectory> origin, const blob& encrypted_data_hash, const blob& block);

	// FSDirectory attachment
	void attach_fs_dir(std::shared_ptr<FSDirectory> fs_dir_ptr);

	// P2PDirectory attachment
	void attach_p2p_dir(std::shared_ptr<P2PDirectory> remote_ptr);
	void detach_p2p_dir(std::shared_ptr<P2PDirectory> remote_ptr);
	bool have_p2p_dir(const tcp_endpoint& endpoint);
	bool have_p2p_dir(const blob& pubkey);

	// Getters
	std::set<std::shared_ptr<FSDirectory>> fs_dirs() const;
	const std::set<std::shared_ptr<P2PDirectory>>& p2p_dirs() const;

	const Key& key() const;
	const blob& hash() const;
private:
	std::shared_ptr<FSDirectory> fs_dir_;
	std::set<std::shared_ptr<P2PDirectory>> p2p_dirs_;
	std::mutex dirs_mtx_;

	mutable std::string name_;
	std::string name() const;
};

} /* namespace librevault */
