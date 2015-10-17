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
#include "../ExchangeGroup.h"

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

std::vector<Meta::PathRevision> FSDirectory::get_meta_list() {
	std::vector<Meta::PathRevision> meta_list;

	for(auto smeta : index->get_Meta()) {
		meta_list.push_back(Meta(smeta.meta_).path_revision());
	}

	return meta_list;
}

void FSDirectory::post_revision(std::shared_ptr<AbstractDirectory> origin, const Meta::PathRevision& revision) {
	try {
		Meta::SignedMeta smeta = index->get_Meta(revision.path_id_);
		Meta meta(smeta.meta_);
		if(meta.revision() == revision.revision_) {
			auto missing_blocks = get_missing_blocks(revision.path_id_);
			if(!missing_blocks.empty()){
				log_->debug() << log_tag() << "Missing " << missing_blocks.size() << " blocks in " << path_id_readable(revision.path_id_);
				for(auto missing_block : missing_blocks){
					origin->request_block(exchange_group_.lock(), missing_block);
				}
			}
		}else if(meta.revision() < revision.revision_) {
			log_->debug() << log_tag() << "Requesting new revision of " << path_id_readable(revision.path_id_);
			origin->request_meta(exchange_group_.lock(), revision.path_id_);
		}
	}catch(Index::no_such_meta& e){
		log_->debug() << log_tag() << "Meta " << path_id_readable(revision.path_id_) << " not found";
		origin->request_meta(exchange_group_.lock(), revision.path_id_);
	}
}

void FSDirectory::request_meta(std::shared_ptr<AbstractDirectory> origin, const blob& meta_id) {
	try {
		Meta::SignedMeta smeta = index->get_Meta(meta_id);
		log_->debug() << log_tag() << "Meta " << path_id_readable(meta_id) << " found and will be sent to " << origin->name();
		origin->post_meta(exchange_group_.lock(), smeta);
	}catch(Index::no_such_meta& e){
		log_->debug() << log_tag() << "Meta " << path_id_readable(meta_id) << " not found";
	}
}

void FSDirectory::post_meta(std::shared_ptr<AbstractDirectory> origin, const Meta::SignedMeta& smeta) {
	log_->debug() << log_tag() << "Received Meta from " << origin->name();

	Meta meta(smeta.meta_);

	// Check for zero-length file. If zero-length and key <= ReadOnly, then assemble.
	index->put_Meta(smeta);	// FIXME: Check revision number, actually
	try {
		open_storage->assemble(meta, true);
	}catch(AbstractStorage::no_such_block& e) {
		for(auto missing_block : get_missing_blocks(meta.path_id())) {
			origin->request_block(exchange_group_.lock(), missing_block);
		}
	}
}

void FSDirectory::request_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id) {
	log_->debug() << log_tag() << "Received block_request from " << origin->name();
	if(open_storage){
		try {
			auto block = open_storage->get_encblock(block_id);
			log_->debug() << log_tag() << "Block " << encrypted_data_hash_readable(block_id) << " found and will be sent to " << origin->name();
			origin->post_block(exchange_group_.lock(), block_id, block);
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
			Meta meta(smeta.meta_);
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

void FSDirectory::set_exchange_group(std::shared_ptr<ExchangeGroup> exchange_group) {
	exchange_group_ = exchange_group;
}

void FSDirectory::handle_smeta(Meta::SignedMeta smeta) {
	Meta::PathRevision path_revision = Meta(smeta.meta_).path_revision();

	log_->debug() << log_tag() << "Created revision " << path_revision.revision_ << " of " << path_id_readable(path_revision.path_id_);

	auto exchange_group_ptr = exchange_group_.lock();
	if(exchange_group_ptr) exchange_group_ptr->broadcast_revision(shared_from_this(), path_revision);
}

std::string FSDirectory::name() const {
	return open_path_.empty() ? block_path_.string() : open_path_.string();
}

} /* namespace librevault */
