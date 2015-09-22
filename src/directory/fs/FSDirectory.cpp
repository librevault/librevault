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

std::vector<FSDirectory::MetaRevision> FSDirectory::get_meta_list() {
	std::vector<MetaRevision> meta_list;

	auto all_smeta = index->get_Meta();
	for(auto smeta : all_smeta) {
		Meta one_meta; one_meta.ParseFromArray(smeta.meta.data(), smeta.meta.size());
		blob path_hmac(one_meta.path_hmac().begin(), one_meta.path_hmac().end());
		meta_list.push_back({path_hmac, one_meta.mtime()});
	}

	return meta_list;
};

void FSDirectory::post_revision(std::shared_ptr<AbstractDirectory> origin, const MetaRevision& revision) {
	try {
		SignedMeta smeta = index->get_Meta(revision.first);
		Meta meta; meta.ParseFromArray(smeta.meta.data(), smeta.meta.size());
		if(meta.mtime() == revision.second){
			auto missing_blocks = get_missing_blocks(revision.first);
			if(!missing_blocks.empty()){
				log_->debug() << log_tag() << "Missing " << missing_blocks.size() << " blocks in " << path_id_readable(revision.first);
				for(auto missing_block : missing_blocks){
					origin->request_block(shared_from_this(), missing_block);
				}
			}
		}else{

		}
	}catch(Index::no_such_meta& e){
		log_->debug() << log_tag() << "Meta " << path_id_readable(revision.first) << " not found";
		origin->request_meta(shared_from_this(), revision.first);
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
	index->put_Meta(smeta);	// FIXME: Check revision number, actually

	Meta m; m.ParseFromArray(smeta.meta.data(), smeta.meta.size());
	blob path_id(m.path_hmac().begin(), m.path_hmac().end());

	for(auto missing_block : get_missing_blocks(path_id)){
		origin->request_block(shared_from_this(), missing_block);
	}
}

void FSDirectory::request_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id) {
	log_->debug() << log_tag() << "Received meta_request from " << origin->name();
	if(open_storage){
		try {
			auto block = open_storage->get_encblock(block_id);
			log_->debug() << log_tag() << "Block " << block_id_readable(block_id) << " found and will be sent to " << origin->name();
			origin->post_block(shared_from_this(), block_id, block);
		}catch(AbstractStorage::no_such_block& e){
			log_->debug() << log_tag() << "Block " << block_id_readable(block_id) << " not found";
		}
	}
}

void FSDirectory::post_block(std::shared_ptr<AbstractDirectory> origin, const blob& block_id, const blob& block) {
	log_->debug() << log_tag() << "Received block " << block_id_readable(block_id) << " from " << origin->name();
	enc_storage->put_encblock(block_id, block);	// FIXME: Check hash!!
	if(open_storage){
		open_storage->assemble(true);
	}
}

std::list<blob> FSDirectory::get_missing_blocks(const blob& path_id) {
	auto sql_result = index->db().exec("SELECT block_encrypted_hash FROM openfs WHERE file_path_hmac=:file_path_hmac AND assembled=0", {
					{":file_path_hmac", path_id}
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
	Meta m; m.ParseFromArray(smeta.meta.data(), smeta.meta.size());
	blob path_hmac(m.path_hmac().begin(), m.path_hmac().end());

	log_->debug() << log_tag() << "Created revision " << m.mtime() << " of " << (std::string)crypto::Base32().to(path_hmac);

	for(auto remote : remotes_){
		remote->post_revision(shared_from_this(), {path_hmac, m.mtime()});
	}
}

std::string FSDirectory::name() const {
	return open_path_.empty() ? block_path_.string() : open_path_.string();
}

} /* namespace librevault */
