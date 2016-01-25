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
#include "FSFolder.h"

#include "IgnoreList.h"
#include "Index.h"
#include "MemoryCachedStorage.h"
#include "EncStorage.h"
#include "OpenStorage.h"
#include "Indexer.h"
#include "AutoIndexer.h"

#include "src/Client.h"
#include "src/folder/FolderGroup.h"

#include "src/util/make_relpath.h"

namespace librevault {

FSFolder::FSFolder(FolderConfig folder_config, Client& client) :
	AbstractFolder(client),
		folder_config_(std::move(folder_config)),
		secret_(folder_config_.secret),

		open_path_(folder_config_.open_path),
		block_path_(!folder_config_.block_path.empty() ? folder_config_.block_path : open_path_ / ".librevault"),
		db_path_(!folder_config_.db_path.empty() ? folder_config_.db_path : block_path_ / "directory.db"),
		asm_path_(!folder_config_.asm_path.empty() ? folder_config_.asm_path : block_path_) {
	name_ = name();
	log_->debug() << log_tag() << "New FSFolder: Key type=" << (char)secret_.get_type();

	ignore_list = std::make_unique<IgnoreList>(*this);
	index = std::make_unique<Index>(*this);

	mem_storage = std::make_unique<MemoryCachedStorage>(*this);
	enc_storage = std::make_unique<EncStorage>(*this);
	if(secret_.get_type() <= Secret::Type::ReadOnly){
		open_storage = std::make_unique<OpenStorage>(*this);
	}
	if(secret_.get_type() <= Secret::Type::ReadWrite){
		indexer = std::make_unique<Indexer>(*this, client_);
		auto_indexer = std::make_unique<AutoIndexer>(*this, client_);
	}
}

FSFolder::~FSFolder() {}

// TODO: rewrite.
bool FSFolder::have_meta(const Meta::PathRevision& path_revision) {
	try {
		get_meta(path_revision);
	}catch(AbstractFolder::no_such_meta& e){
		return false;
	}
	return true;
}

Meta::SignedMeta FSFolder::get_meta(const Meta::PathRevision& path_revision) {
	auto smeta = index->get_Meta(path_revision.path_id_);
	if(smeta.meta().revision() == path_revision.revision_)
		return smeta;
	else throw AbstractFolder::no_such_meta();
}

std::list<Meta::SignedMeta> FSFolder::get_meta_containing(const blob& encrypted_data_hash) {
	return index->containing_block(encrypted_data_hash);
}

void FSFolder::put_meta(Meta::SignedMeta smeta, bool fully_assembled) {
	auto path_revision = smeta.meta().path_revision();

	index->put_Meta(smeta, fully_assembled);

	bitfield_type bitfield;
	if(!fully_assembled) {
		bitfield = make_bitfield(smeta.meta());
		if(bitfield.all()) {
			open_storage->assemble(smeta.meta(), true);
		}
	}else{
		bitfield.resize(smeta.meta().blocks().size(), true);
	}

	exchange_group_.lock()->notify_meta(shared_from_this(), path_revision, bitfield);
}

blob FSFolder::get_chunk(const blob& encrypted_data_hash, uint32_t offset, uint32_t size) {
	auto block = get_block(encrypted_data_hash);
	if(offset < block.size() && size <= block.size()-offset)
		return blob(block.begin()+offset, block.begin()+offset+size);
	else
		throw AbstractFolder::no_such_block();
}

bool FSFolder::have_block(const blob& encrypted_data_hash) const {
	return enc_storage->have_block(encrypted_data_hash) || open_storage->have_block(encrypted_data_hash);
}

blob FSFolder::get_block(const blob& encrypted_data_hash) {
	try {
		// Cache hit
		return *mem_storage->get_block(encrypted_data_hash);
	}catch(AbstractFolder::no_such_block& e) {
		// Cache missed
		std::shared_ptr<blob> block_ptr;
		try {
			block_ptr = enc_storage->get_block(encrypted_data_hash);
		}catch(AbstractFolder::no_such_block& e) {
			block_ptr = open_storage->get_block(encrypted_data_hash);
		}
		mem_storage->put_block(encrypted_data_hash, block_ptr);
		return *block_ptr;
	}
}

void FSFolder::put_block(const blob& encrypted_data_hash, const blob& block) {
	enc_storage->put_block(encrypted_data_hash, block);
	exchange_group_.lock()->notify_block(shared_from_this(), encrypted_data_hash);

	if(open_storage) {
		auto meta_list = get_meta_containing(encrypted_data_hash);
		for(auto& smeta : meta_list) {
			auto bitfield = make_bitfield(smeta.meta());
			if(bitfield.all()) {
				open_storage->assemble(smeta.meta(), true);
			}
		}
	}
}

// TODO: Maybe, add some caching
bitfield_type FSFolder::get_bitfield(const Meta::PathRevision& path_revision) {
	return make_bitfield(get_meta(path_revision).meta());
}

/* Makers */
std::string FSFolder::make_relpath(const fs::path& path) const {
	return ::librevault::make_relpath(path, open_path());
}

bitfield_type FSFolder::make_bitfield(const Meta& meta) const {
	bitfield_type bitfield(meta.blocks().size());

	for(unsigned int bitfield_idx = 0; bitfield_idx < meta.blocks().size(); bitfield_idx++)
		if(have_block(meta.blocks().at(bitfield_idx).encrypted_data_hash_))
			bitfield[bitfield_idx] = true;

	return bitfield;
}

/* Getters */
std::string FSFolder::name() const {
	return open_path_.empty() ? block_path_.string() : open_path_.string();
}

} /* namespace librevault */
