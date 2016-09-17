/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "FSFolder.h"

#include "IgnoreList.h"
#include "Index.h"
#include "folder/fs/chunk/ChunkStorage.h"
#include "Indexer.h"
#include "AutoIndexer.h"

#include "Client.h"

#include "util/make_relpath.h"
#include "util/log.h"

#include <boost/locale.hpp>
#include <codecvt>

namespace librevault {

FSFolder::FSFolder(FolderGroup& group, Client& client) :
	group_(group)  {
	// Creating directories
	bool path_created = fs::create_directories(params().path);
	bool system_path_created = fs::create_directories(params().system_path);
#if BOOST_OS_WINDOWS
	SetFileAttributes(params().system_path.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

	name_ = (!params().path.empty() ? params().path : params().system_path).string();
	LOGD("New FSFolder:"
		<< " Key type=" << (char)params().secret.get_type()
		<< " Path" << (path_created ? " created" : "") << "=" << params().path
		<< " System path" << (system_path_created ? " created" : "") << "=" << params().system_path);

	ignore_list = std::make_unique<IgnoreList>(*this);
	index = std::make_unique<Index>(*this, client);

	chunk_storage = std::make_unique<ChunkStorage>(*this, client);

	if(params().secret.get_type() <= Secret::Type::ReadWrite){
		indexer = std::make_unique<Indexer>(*this, client);
		auto_indexer = std::make_unique<AutoIndexer>(*this, client);
	}
}

FSFolder::~FSFolder() {}

bool FSFolder::have_meta(const Meta::PathRevision& path_revision) noexcept {
	return index->have_meta(path_revision);
}

SignedMeta FSFolder::get_meta(const Meta::PathRevision& path_revision) {
	return index->get_meta(path_revision);
}

void FSFolder::put_meta(SignedMeta smeta, bool fully_assembled) {
	index->put_meta(smeta, fully_assembled);
}

bool FSFolder::have_chunk(const blob& ct_hash) const noexcept {
	return chunk_storage->have_chunk(ct_hash);
}

blob FSFolder::get_chunk(const blob& ct_hash) {
	return chunk_storage->get_chunk(ct_hash);
}

void FSFolder::put_chunk(const blob& ct_hash, const fs::path& chunk_location) {
	chunk_storage->put_chunk(ct_hash, chunk_location);
}

bitfield_type FSFolder::get_bitfield(const Meta::PathRevision& path_revision) {
	return chunk_storage->make_bitfield(get_meta(path_revision).meta());
}

/* Makers */
std::string FSFolder::normalize_path(const fs::path& abspath) const {
#ifdef DEBUG_NORMALIZATION
	log_->debug() << log_tag() << BOOST_CURRENT_FUNCTION << " " << abspath;
#endif

	// Relative path in platform-independent format
	fs::path rel_path = ::librevault::make_relpath(abspath, path());

	std::string norm_path = rel_path.generic_string(std::codecvt_utf8_utf16<wchar_t>());
	if(params().normalize_unicode)	// Unicode normalization NFC (for compatibility)
		norm_path = boost::locale::normalize(norm_path, boost::locale::norm_nfc);

	// Removing last '/' in directories
	if(norm_path.size() > 0 && norm_path.back() == '/')
		norm_path.pop_back();

#ifdef DEBUG_NORMALIZATION
	log_->debug() << log_tag() << BOOST_CURRENT_FUNCTION << " " << norm_path;
#endif
	return norm_path;
}

fs::path FSFolder::absolute_path(const std::string& normpath) const {
#ifdef DEBUG_NORMALIZATION
	log_->debug() << log_tag() << BOOST_CURRENT_FUNCTION << " " << normpath;
#endif

#if BOOST_OS_WINDOWS
	std::wstring osnormpath = boost::locale::conv::utf_to_utf<wchar_t>(normpath);
#else
	const std::string& osnormpath(normpath);
#endif
	fs::path abspath = path() / osnormpath;

#ifdef DEBUG_NORMALIZATION
	log_->debug() << log_tag() << BOOST_CURRENT_FUNCTION << " " << abspath;
#endif
	return abspath;
}

} /* namespace librevault */
