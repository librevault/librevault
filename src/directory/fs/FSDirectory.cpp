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
#include "FSDirectory.h"

#include "../../Session.h"
#include "../../net/parse_url.h"
#include "../Exchanger.h"
#include "../p2p/P2PDirectory.h"

namespace librevault {

FSDirectory::FSDirectory(ptree dir_options, Session& session, Exchanger& exchanger) :
		AbstractDirectory(session, exchanger),
		dir_options_(std::move(dir_options)),
		key_(dir_options_.get<std::string>("key")),

		open_path_(dir_options_.get<fs::path>("open_path")),
		block_path_(dir_options_.get<fs::path>("block_path", open_path_ / ".librevault")),
		db_path_(dir_options_.get<fs::path>("db_path", block_path_ / "directory.db")),
		asm_path_(dir_options_.get<fs::path>("asm_path", block_path_ / "assembled.part")) {
	log_->debug() << log_tag() << "New FSDirectory: Key type=" << (char)key_.get_type();

	index = std::make_unique<Index>(*this, session);
	if(key_.get_type() <= Key::Type::Download){
		enc_storage = std::make_unique<EncStorage>(*this, session_);
	}
	if(key_.get_type() <= Key::Type::ReadOnly){
		open_storage = std::make_unique<OpenStorage>(*this, session_);
	}
	if(key_.get_type() <= Key::Type::ReadWrite){
		indexer = std::make_unique<Indexer>(*this, session_);
		auto_indexer = std::make_unique<AutoIndexer>(*this, session_, std::bind(&FSDirectory::handle_smeta, this, std::placeholders::_1));
	}
}

FSDirectory::~FSDirectory() {}

void FSDirectory::attach_remote(std::shared_ptr<P2PDirectory> remote_ptr) {
	remotes_.insert(remote_ptr);
	log_->debug() << log_tag() << "Attached remote " << remote_ptr->remote_string() << " to " << name();
}

void FSDirectory::detach_remote(std::shared_ptr<P2PDirectory> remote_ptr) {
	remotes_.erase(remote_ptr);
	log_->debug() << log_tag() << "Detached remote " << remote_ptr->remote_string() << " from " << name();
}

std::vector<Meta::PathRevision> FSDirectory::get_meta_list() {
	std::vector<Meta::PathRevision> meta_list;

	auto all_smeta = index->get_Meta();
	for(auto smeta : all_smeta) {
		Meta one_meta(smeta.meta);
		meta_list.push_back(one_meta.path_revision());
	}

	return meta_list;
}

void FSDirectory::post_revision(std::shared_ptr<AbstractDirectory> origin, const Meta::PathRevision& revision) {
	try {
		SignedMeta smeta = index->get_Meta(revision.path_id_);
		Meta meta(smeta.meta);
		if(meta.revision() == revision.revision_) {
			auto missing_blocks = get_missing_blocks(revision.path_id_);
			if(!missing_blocks.empty()){
				log_->debug() << log_tag() << "Missing " << missing_blocks.size() << " blocks in " << path_id_readable(revision.path_id_);
				for(auto missing_block : missing_blocks){
					origin->request_block(shared_from_this(), missing_block);
				}
			}
		}else if(meta.revision() < revision.revision_) {
			log_->debug() << log_tag() << "Requesting new revision of " << path_id_readable(revision.path_id_);
			origin->request_meta(shared_from_this(), revision.path_id_);
		}
	}catch(Index::no_such_meta& e){
		log_->debug() << log_tag() << "Meta " << path_id_readable(revision.path_id_) << " not found";
		origin->request_meta(shared_from_this(), revision.path_id_);
	}
}

void FSDirectory::request_meta(std::shared_ptr<AbstractDirectory> origin, const blob& meta_id) {
	try {
		SignedMeta smeta = index->get_Meta(meta_id);
		log_->debug() << log_tag() << "Meta " << path_id_readable(meta_id) << " found and will be sent to " << origin->name();
		origin->post_meta(shared_from_this(), smeta);
	}catch(Index::no_such_meta& e){
		log_->debug() << log_tag() << "Meta " << path_id_readable(meta_id) << " not found";
	}
}

void FSDirectory::post_meta(std::shared_ptr<AbstractDirectory> origin, const SignedMeta& smeta) {
	log_->debug() << log_tag() << "Received Meta from " << origin->name();

	Meta meta(smeta.meta);

	// Check for zero-length file. If zero-length and key <= ReadOnly, then assemble.
	index->put_Meta(smeta);	// FIXME: Check revision number, actually
	try {
		open_storage->assemble(meta, true);
	}catch(AbstractStorage::no_such_block& e) {
		for(auto missing_block : get_missing_blocks(meta.path_id())) {
			origin->request_block(shared_from_this(), missing_block);
		}
	}
}

void FSDirectory::request_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id) {
	log_->debug() << log_tag() << "Received block_request from " << origin->name();
	if(open_storage){
		try {
			auto block = open_storage->get_encblock(block_id);
			log_->debug() << log_tag() << "Block " << encrypted_data_hash_readable(block_id) << " found and will be sent to " << origin->name();
			origin->post_block(shared_from_this(), block_id, block);
		}catch(AbstractStorage::no_such_block& e){
			log_->debug() << log_tag() << "Block " << encrypted_data_hash_readable(block_id) << " not found";
		}
	}
}

void FSDirectory::post_block(std::shared_ptr<AbstractDirectory> origin, const blob& encrypted_data_hash, const blob& block) {
	log_->debug() << log_tag() << "Received block " << encrypted_data_hash_readable(encrypted_data_hash) << " from " << origin->name();
	enc_storage->put_encblock(encrypted_data_hash, block);	// FIXME: Check hash!!
	if(open_storage){
		for(auto& smeta : index->containing_block(encrypted_data_hash)) {
			Meta meta(smeta.meta);
			try {
				open_storage->assemble(meta, true);
			}catch(AbstractStorage::no_such_block& e){
				log_->debug() << log_tag() << "Not enough blocks for assembling " << encrypted_data_hash_readable(encrypted_data_hash);
			}
		}
	}
}

std::list<blob> FSDirectory::get_missing_blocks(const blob& path_id) {
	auto sql_result = index->db().exec("SELECT encrypted_data_hash FROM openfs WHERE path_id=:path_id AND assembled=0", {
					{":path_id", path_id}
			});

	std::list<blob> missing_blocks;

	for(auto row : sql_result){
		blob block_id = row[0];
		if(!enc_storage->have_encblock(block_id)){
			missing_blocks.push_back(block_id);
		}
	}

	return missing_blocks;
}

void FSDirectory::handle_smeta(AbstractDirectory::SignedMeta smeta) {
	Meta m; m.parse(smeta.meta);
	blob path_id(m.path_id());

	log_->debug() << log_tag() << "Created revision " << m.revision() << " of " << crypto::Base32().to_string(path_id);

	for(auto remote : remotes_){
		remote->post_revision(shared_from_this(), {path_id, m.revision()});
	}
}

std::string FSDirectory::name() const {
	return open_path_.empty() ? block_path_.string() : open_path_.string();
}

} /* namespace librevault */
