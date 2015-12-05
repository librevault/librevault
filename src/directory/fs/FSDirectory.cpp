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
#include "FSDirectory.h"

#include "../../Client.h"
#include "../../net/parse_url.h"
#include "../Exchanger.h"
#include "../ExchangeGroup.h"

namespace librevault {

FSDirectory::FSDirectory(ptree dir_options, Client& client, Exchanger& exchanger) :
		AbstractDirectory(client, exchanger),
		dir_options_(std::move(dir_options)),
		key_(dir_options_.get<std::string>("key")),

		open_path_(dir_options_.get<fs::path>("open_path")),
		block_path_(dir_options_.get<fs::path>("block_path", open_path_ / ".librevault")),
		db_path_(dir_options_.get<fs::path>("db_path", block_path_ / "directory.db")),
		asm_path_(dir_options_.get<fs::path>("asm_path", block_path_ / "assembled.part")) {
	log_->debug() << log_tag() << "New FSDirectory: Key type=" << (char)key_.get_type();

	ignore_list = std::make_unique<IgnoreList>(*this, client_);
	index = std::make_unique<Index>(*this, client_);
	enc_storage = std::make_unique<EncStorage>(*this, client_);
	chunk_storage = std::make_unique<ChunkStorage>(*this, client_);
	if(key_.get_type() <= Key::Type::ReadOnly){
		open_storage = std::make_unique<OpenStorage>(*this, client_);
	}
	if(key_.get_type() <= Key::Type::ReadWrite){
		indexer = std::make_unique<Indexer>(*this, client_);
		auto_indexer = std::make_unique<AutoIndexer>(*this, client_, std::bind(&FSDirectory::handle_smeta, this, std::placeholders::_1));
	}
}

Meta FSDirectory::get_meta(const Meta::PathRevision& path_revision) {
	auto meta = Meta(index->get_Meta(path_revision.path_id_).meta_);
	if(meta.revision() == path_revision.revision_)
		return meta;
	else throw Index::no_such_meta();
}

void FSDirectory::put_meta(const Meta::SignedMeta& smeta, bool fully_assembled) {
	std::unique_lock<std::shared_timed_mutex> lk(path_id_info_mtx_);
	Meta meta(smeta.meta_);
	auto path_revision = meta.path_revision();
	if(will_accept_meta(path_revision)) {
		index->put_Meta(smeta, fully_assembled);

		auto bitfield = make_bitfield(meta);

		path_id_info_[path_revision.path_id_] = {path_revision.revision_, bitfield};
		exchange_group_.lock()->post_have_meta(shared_from_this(), path_revision, bitfield);
	}
}

blob FSDirectory::get_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t size) {
	auto block = get_block(encrypted_data_hash);
	if(offset < block.size() && size <= block.size()-offset)
		return blob(block.begin()+offset, block.begin()+offset+size);
	else
		throw AbstractStorage::no_such_block();
}

void FSDirectory::put_chunk(const blob& encrypted_data_hash, uint32_t offset, const blob& chunk) {
	chunk_storage->put_chunk(encrypted_data_hash, offset, chunk);
}

bool FSDirectory::have_block(const blob& encrypted_data_hash) {
	return enc_storage->have_block(encrypted_data_hash) || open_storage->have_block(encrypted_data_hash);
}

blob FSDirectory::get_block(const blob& encrypted_data_hash) {
	try {
		return enc_storage->get_block(encrypted_data_hash);
	}catch(AbstractStorage::no_such_block& e){
		return open_storage->get_block(encrypted_data_hash);
	}
}

void FSDirectory::put_block(const blob& encrypted_data_hash, const blob& block) {
	enc_storage->put_block(encrypted_data_hash, block);
	exchange_group_.lock()->post_have_block(shared_from_this(), encrypted_data_hash);
}

/* Makers */
std::string FSDirectory::make_relpath(const fs::path& path) const {
	fs::path rel_to = open_path();
	auto abspath = fs::absolute(path);

	fs::path relpath;
	auto path_elem_it = abspath.begin();
	for(auto dir_elem : rel_to){
		if(dir_elem != *(path_elem_it++))
			return std::string();
	}
	for(; path_elem_it != abspath.end(); path_elem_it++){
		if(*path_elem_it == "." || *path_elem_it == "..")
			return std::string();
		relpath /= *path_elem_it;
	}
	return relpath.generic_string();
}

AbstractDirectory::bitfield_type FSDirectory::make_bitfield(const Meta& meta) {
	bitfield_type bitfield(meta.blocks().size());

	for(unsigned int bitfield_idx = 0; bitfield_idx < meta.blocks().size(); bitfield_idx++)
		if(have_block(meta.blocks().at(bitfield_idx).encrypted_data_hash_))
			bitfield[bitfield_idx] = true;

	return bitfield;
}

/* Getters */
std::string FSDirectory::name() const {
	return open_path_.empty() ? block_path_.string() : open_path_.string();
}

void FSDirectory::handle_smeta(Meta::SignedMeta smeta) {
	std::unique_lock<std::shared_timed_mutex> lk(path_id_info_mtx_);
	Meta meta(smeta.meta_);
	auto path_revision = meta.path_revision();
	bitfield_type bitfield; bitfield.resize(meta.blocks().size(), true);

	path_id_info_[path_revision.path_id_] = {path_revision.revision_, bitfield};

	auto group_ptr = exchange_group_.lock();
	if(group_ptr)
		group_ptr->post_have_meta(shared_from_this(), path_revision, bitfield);
}

} /* namespace librevault */
