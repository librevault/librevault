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
#include "IndexerWorker.h"

#include "Index.h"
#include "MetaStorage.h"
#include "control/FolderParams.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"
#include "human_size.h"
#include <librevault/crypto/HMAC-SHA3.h>
#include <librevault/crypto/AES_CBC.h>
#include <rabin.h>
#include <boost/filesystem.hpp>
#include <QFile>
#ifdef Q_OS_UNIX
#   include <sys/stat.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(log_indexerworker)

namespace librevault {

IndexerWorker::IndexerWorker(QString abspath, const FolderParams& params, Index* index, IgnoreList* ignore_list, PathNormalizer* path_normalizer, QObject* parent) :
	QObject(parent),
	abspath_(abspath),
	params_(params),
	index_(index),
	ignore_list_(ignore_list),
	path_normalizer_(path_normalizer),
	secret_(params.secret),
	active_(true) {}

IndexerWorker::~IndexerWorker() {}

void IndexerWorker::run() noexcept {
	QByteArray normpath = path_normalizer_->normalizePath(abspath_);
	qCDebug(log_indexerworker) << "Started indexing:" << normpath;

	try {
		if(ignore_list_->isIgnored(normpath)) throw abort_index("File is ignored");

		try {
			old_smeta_ = index_->get_meta(Meta::make_path_id(normpath.toStdString(), secret_));
			old_meta_ = old_smeta_.meta();
			if(boost::filesystem::last_write_time(abspath_.toStdString()) == old_meta_.mtime()) {
				throw abort_index("Modification time is not changed");
			}
		}catch(boost::filesystem::filesystem_error& e){
		}catch(MetaStorage::no_such_meta& e){
		}catch(Meta::error& e){
			qCDebug(log_indexerworker) << "Meta in DB is inconsistent, trying to reindex:" << e.what();
		}

		std::chrono::high_resolution_clock::time_point before_index = std::chrono::high_resolution_clock::now();  // Starting timer
		make_Meta();   // Actual indexing
		std::chrono::high_resolution_clock::time_point after_index = std::chrono::high_resolution_clock::now();   // Stopping timer
		float time_spent = std::chrono::duration<float, std::chrono::seconds::period>(after_index - before_index).count();

		qCDebug(log_indexerworker) << "Updated index entry in" << time_spent << "s (" << human_bandwidth((double)new_smeta_.meta().size()/time_spent) << ")"
			<< "Path=" << abspath_
			<< "Rev=" << new_smeta_.meta().revision()
			<< "Chk=" << new_smeta_.meta().chunks().size();

		emit metaCreated(new_smeta_);
	}catch(std::runtime_error& e){
		emit metaFailed(e.what());
	}
}

/* Actual indexing process */
void IndexerWorker::make_Meta() {
	QString abspath = abspath_;
	QByteArray normpath = path_normalizer_->normalizePath(abspath);

	//LOGD("make_Meta(" << normpath.toStdString() << ")");

	new_meta_.set_path(normpath.toStdString(), secret_);    // sets path_id, encrypted_path and encrypted_path_iv

	new_meta_.set_meta_type(get_type());  // Type

	if(!old_smeta_ && new_meta_.meta_type() == Meta::DELETED)
		throw abort_index("Old Meta is not in the index, new Meta is DELETED");

	if(old_meta_.meta_type() == Meta::DIRECTORY && new_meta_.meta_type() == Meta::DIRECTORY)
		throw abort_index("Old Meta is DIRECTORY, new Meta is DIRECTORY");

	if(old_meta_.meta_type() == Meta::DELETED && new_meta_.meta_type() == Meta::DELETED)
		throw abort_index("Old Meta is DELETED, new Meta is DELETED");

	if(new_meta_.meta_type() == Meta::FILE)
		update_chunks();

	if(new_meta_.meta_type() == Meta::SYMLINK) {
		new_meta_.set_symlink_path(boost::filesystem::read_symlink(abspath_.toStdWString()).generic_string(), secret_);
	}

	// FSAttrib
	if(new_meta_.meta_type() != Meta::DELETED)
		update_fsattrib();   // Platform-dependent attributes (windows attrib, uid, gid, mode)

	// Revision
	new_meta_.set_revision(std::time(nullptr));	// Meta is ready. Assigning timestamp.

	new_smeta_ = SignedMeta(new_meta_, secret_);
}

Meta::Type IndexerWorker::get_type() {
	QString abspath = abspath_;
	boost::filesystem::path babspath(abspath.toStdWString());

	boost::filesystem::file_status file_status = params_.preserve_symlinks ? boost::filesystem::symlink_status(babspath) : boost::filesystem::status(babspath);	// Preserves symlinks if such option is set.

	switch(file_status.type()){
		case boost::filesystem::regular_file: return Meta::FILE;
		case boost::filesystem::directory_file: return Meta::DIRECTORY;
		case boost::filesystem::symlink_file: return Meta::SYMLINK;
		case boost::filesystem::file_not_found: return Meta::DELETED;
		default: throw abort_index("File type is unsuitable for indexing. Only Files, Directories and Symbolic links are supported");
	}
}

void IndexerWorker::update_fsattrib() {
	boost::filesystem::path babspath(abspath_.toStdWString());

	// First, preserve old values of attributes
	new_meta_.set_windows_attrib(old_meta_.windows_attrib());
	new_meta_.set_mode(old_meta_.mode());
	new_meta_.set_uid(old_meta_.uid());
	new_meta_.set_gid(old_meta_.gid());

	if(new_meta_.meta_type() != Meta::SYMLINK)
		new_meta_.set_mtime(boost::filesystem::last_write_time(babspath));   // File/directory modification time
	else {
		// TODO: make alternative function for symlinks. Use boost::filesystem::last_write_time as an example. lstat for Unix and GetFileAttributesEx for Windows.
	}

	// Then, write new values of attributes (if enabled in config)
#if defined(Q_OS_WIN)
	if(params_.preserve_windows_attrib) {
		new_meta_.set_windows_attrib(GetFileAttributes(babspath.native().c_str()));	// Windows attributes (I don't have Windows now to test it), this code is stub for now.
	}
#elif defined(Q_OS_UNIX)
	if(params_.preserve_unix_attrib) {
		struct stat stat_buf; int stat_err = 0;
		if(params_.preserve_symlinks)
			stat_err = lstat(babspath.c_str(), &stat_buf);
		else
			stat_err = stat(babspath.c_str(), &stat_buf);
		if(stat_err == 0){
			new_meta_.set_mode(stat_buf.st_mode);
			new_meta_.set_uid(stat_buf.st_uid);
			new_meta_.set_gid(stat_buf.st_gid);
		}
	}
#endif
}

void IndexerWorker::update_chunks() {
	Meta::RabinGlobalParams rabin_global_params;

	if(old_meta_.meta_type() == Meta::FILE && old_meta_.validate()) {
		new_meta_.set_algorithm_type(old_meta_.algorithm_type());
		new_meta_.set_strong_hash_type(old_meta_.strong_hash_type());

		new_meta_.set_max_chunksize(old_meta_.max_chunksize());
		new_meta_.set_min_chunksize(old_meta_.min_chunksize());

		new_meta_.raw_rabin_global_params() = old_meta_.raw_rabin_global_params();

		rabin_global_params = old_meta_.rabin_global_params(secret_);
	}else{
		new_meta_.set_algorithm_type(Meta::RABIN);
		new_meta_.set_strong_hash_type(params_.chunk_strong_hash_type);

		new_meta_.set_max_chunksize(8*1024*1024);
		new_meta_.set_min_chunksize(1*1024*1024);

		// TODO: Generate a new polynomial for rabin_global_params here to prevent a possible fingerprinting attack.
	}

	// IV reuse
	std::map<blob, blob> pt_hmac__iv;
	for(auto& chunk : old_meta_.chunks()) {
		pt_hmac__iv.insert({chunk.pt_hmac, chunk.iv});
	}

	// Initializing chunker
	rabin_t hasher;
	hasher.average_bits = rabin_global_params.avg_bits;
	hasher.minsize = new_meta_.min_chunksize();
	hasher.maxsize = new_meta_.max_chunksize();
	hasher.polynomial = rabin_global_params.polynomial;
	hasher.polynomial_degree = rabin_global_params.polynomial_degree;
	hasher.polynomial_shift = rabin_global_params.polynomial_shift;

	hasher.mask = uint64_t((1<<uint64_t(hasher.average_bits))-1);

	rabin_init(&hasher);

	// Chunking
	std::vector<Meta::Chunk> chunks;

	blob buffer;
	buffer.reserve(hasher.maxsize);

	QFile f(abspath_);
	if(!f.open(QIODevice::ReadOnly))
		throw abort_index("I/O error: " + f.errorString());

	char byte;
	while(f.getChar(&byte) && active_) {
		buffer.push_back(byte);
		//size_t len = fread(buf, 1, sizeof(buf), stdin);
		uint8_t *ptr = &buffer.back();

		if(rabin_next_chunk(&hasher, ptr, 1) == 1) {    // Found a chunk
			chunks.push_back(populate_chunk(buffer, pt_hmac__iv));
			buffer.clear();
		}
	}

	if(!active_)
		throw abort_index("Indexing had been interruped");

	if(rabin_finalize(&hasher) != 0)
		chunks.push_back(populate_chunk(buffer, pt_hmac__iv));

	new_meta_.set_chunks(chunks);
}

Meta::Chunk IndexerWorker::populate_chunk(const blob& data, const std::map<blob, blob>& pt_hmac__iv) {
	qCDebug(log_indexerworker) << "New chunk size:" << data.size();
	Meta::Chunk chunk;
	chunk.pt_hmac = data | crypto::HMAC_SHA3_224(secret_.get_Encryption_Key());

	// IV reuse
	auto it = pt_hmac__iv.find(chunk.pt_hmac);
	chunk.iv = (it != pt_hmac__iv.end() ? it->second : crypto::AES_CBC::random_iv());

	chunk.size = data.size();
	chunk.ct_hash = Meta::Chunk::compute_strong_hash(Meta::Chunk::encrypt(data, secret_.get_Encryption_Key(), chunk.iv), new_meta_.strong_hash_type());
	return chunk;
}

} /* namespace librevault */
