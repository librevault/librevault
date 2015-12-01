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
#include "Key.h"
#include "AbstractDirectory.h"

namespace librevault {

class Client;
class Exchanger;

class RemoteDirectory;
class FSDirectory;
class P2PDirectory;

class ExchangeGroup : public std::enable_shared_from_this<ExchangeGroup>, protected Loggable {
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("ExchangeGroup error") {}
	};

	struct attach_error : error {
		attach_error() : error("Could not attach remote to ExchangeGroup") {}
	};

	ExchangeGroup(Client& client, Exchanger& exchanger);

	/* Actions */
	void post_have_meta(std::shared_ptr<FSDirectory> origin, const Meta::PathRevision& revision, const AbstractDirectory::bitfield_type& bitfield);
	void post_have_block(std::shared_ptr<FSDirectory> origin, const blob& encrypted_data_hash);

	/* Membership management */
	void attach(std::shared_ptr<FSDirectory> fs_dir_ptr);
	void attach(std::shared_ptr<P2PDirectory> remote_ptr);

	void detach(std::shared_ptr<P2PDirectory> remote_ptr);

	bool have_p2p_dir(const tcp_endpoint& endpoint);
	bool have_p2p_dir(const blob& pubkey);

	/* Getters */
	std::set<std::shared_ptr<FSDirectory>> fs_dirs() const;
	const std::set<std::shared_ptr<P2PDirectory>>& p2p_dirs() const;

	const Key& key() const;
	const blob& hash() const;
private:
	Client& client_;
	Exchanger& exchanger_;

	/* Members */
	std::shared_ptr<FSDirectory> fs_dir_;
	std::set<std::shared_ptr<P2PDirectory>> p2p_dirs_;
	mutable std::shared_timed_mutex dirs_mtx_;

	/* Lookup optimization */
	std::set<blob> p2p_dirs_pubkeys_;
	std::set<tcp_endpoint> p2p_dirs_endpoints_;

	std::string name() const {return name_;}
};

} /* namespace librevault */
