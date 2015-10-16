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
#pragma once
#include "../../pch.h"
#include "../Abstract.h"

#include "../Key.h"
#include "Index.h"
#include "EncStorage.h"
#include "OpenStorage.h"
#include "Indexer.h"
#include "AutoIndexer.h"

namespace librevault {

class P2PDirectory;
class FSDirectory : public AbstractDirectory, public std::enable_shared_from_this<FSDirectory> {
public:
	FSDirectory(ptree dir_options, Session& session, Exchanger& exchanger);
	virtual ~FSDirectory();

	// Remote management
	void attach_p2p_dir(std::shared_ptr<P2PDirectory> remote_ptr);
	void detach_p2p_dir(std::shared_ptr<P2PDirectory> remote_ptr);
	bool have_p2p_dir(const tcp_endpoint& endpoint);
	bool have_p2p_dir(const blob& pubkey);

	std::list<blob> get_missing_blocks(const blob& encrypted_data_hash);

	// AbstractDirectory actions
	std::vector<Meta::PathRevision> get_meta_list();

	void post_revision(std::shared_ptr<AbstractDirectory> origin, const Meta::PathRevision& revision);
	void request_meta(std::shared_ptr<AbstractDirectory> origin, const blob& path_id);
	void post_meta(std::shared_ptr<AbstractDirectory> origin, const SignedMeta& smeta);
	void request_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id);
	void post_block(std::shared_ptr<AbstractDirectory> origin, const blob& encrypted_data_hash, const blob& block);

	// Getters
	const ptree& dir_options() const {return dir_options_;}

	const Key& key() const {return key_;}
	const blob& hash() const {return key().get_Hash();}
	std::string name() const;

	const fs::path& open_path() const {return open_path_;}
	const fs::path& block_path() const {return block_path_;}
	const fs::path& db_path() const {return db_path_;}
	const fs::path& asm_path() const {return asm_path_;}

	// Components
	std::unique_ptr<Index> index;
	std::unique_ptr<EncStorage> enc_storage;
	std::unique_ptr<OpenStorage> open_storage;
	std::unique_ptr<Indexer> indexer;
	std::unique_ptr<AutoIndexer> auto_indexer;

private:
	ptree dir_options_;
	const Key key_;
	const fs::path open_path_, block_path_, db_path_, asm_path_;	// Paths

	std::set<std::shared_ptr<P2PDirectory>> remotes_;
	std::mutex remotes_mtx_;

	// Revision operations
	void handle_smeta(AbstractDirectory::SignedMeta smeta);
};

} /* namespace librevault */
