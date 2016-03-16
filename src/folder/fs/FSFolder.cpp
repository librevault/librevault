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

FSFolder::FSFolder(FolderGroup& group, Client& client) :
	AbstractFolder(client),
	group_(group)  {
	// Creating directories
	bool path_created = fs::create_directories(params().path);
	bool system_path_created = fs::create_directories(params().system_path);
#if BOOST_OS_WINDOWS
	SetFileAttributes(params().system_path.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

	name_ = (!params().path.empty() ? params().path : params().system_path).string();
	log_->debug() << log_tag() << "New FSFolder:"
		<< " Key type=" << (char)params().secret.get_type()
		<< " Path" << (path_created ? " created" : "") << "=" << params().path
		<< " System path" << (system_path_created ? " created" : "") << "=" << params().system_path;

	ignore_list = std::make_unique<IgnoreList>(*this);
	index = std::make_unique<Index>(*this);

	mem_storage = std::make_unique<MemoryCachedStorage>(*this);
	enc_storage = std::make_unique<EncStorage>(*this);
	if(params().secret.get_type() <= Secret::Type::ReadOnly){
		open_storage = std::make_unique<OpenStorage>(*this);
	}
	if(params().secret.get_type() <= Secret::Type::ReadWrite){
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

SignedMeta FSFolder::get_meta(const Meta::PathRevision& path_revision) {
	auto smeta = index->get_Meta(path_revision.path_id_);
	if(smeta.meta().revision() == path_revision.revision_)
		return smeta;
	else throw AbstractFolder::no_such_meta();
}

std::list<SignedMeta> FSFolder::get_meta_containing(const blob& ct_hash) {
	return index->containing_chunk(ct_hash);
}

void FSFolder::put_meta(SignedMeta smeta, bool fully_assembled) {
	auto path_revision = smeta.meta().path_revision();

	index->put_Meta(smeta, fully_assembled);

	bitfield_type bitfield;
	if(!fully_assembled) {
		bitfield = make_bitfield(smeta.meta());
		if(bitfield.all()) {
			open_storage->assemble(smeta.meta(), true);
		}
	}else{
		bitfield.resize(smeta.meta().chunks().size(), true);
	}

	group_.notify_meta(shared_from_this(), path_revision, bitfield);
}

bool FSFolder::have_chunk(const blob& ct_hash) const {
	return enc_storage->have_chunk(ct_hash) || open_storage->have_chunk(ct_hash);
}

blob FSFolder::get_chunk(const blob& ct_hash) {
	try {
		// Cache hit
		return *mem_storage->get_chunk(ct_hash);
	}catch(AbstractFolder::no_such_chunk& e) {
		// Cache missed
		std::shared_ptr<blob> block_ptr;
		try {
			block_ptr = enc_storage->get_chunk(ct_hash);
		}catch(AbstractFolder::no_such_chunk& e) {
			block_ptr = open_storage->get_chunk(ct_hash);
		}
		mem_storage->put_chunk(ct_hash, block_ptr);
		return *block_ptr;
	}
}

void FSFolder::put_chunk(const blob& ct_hash, const blob& chunk) {
	enc_storage->put_chunk(ct_hash, chunk);
	group_.notify_chunk(shared_from_this(), ct_hash);

	if(open_storage) {
		auto meta_list = get_meta_containing(ct_hash);
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
std::string FSFolder::make_relpath(const fs::path& abspath) const {
	return ::librevault::make_relpath(abspath, path());
}

bitfield_type FSFolder::make_bitfield(const Meta& meta) const {
	bitfield_type bitfield(meta.chunks().size());

	for(unsigned int bitfield_idx = 0; bitfield_idx < meta.chunks().size(); bitfield_idx++)
		if(have_chunk(meta.chunks().at(bitfield_idx).ct_hash))
			bitfield[bitfield_idx] = true;

	return bitfield;
}

} /* namespace librevault */
