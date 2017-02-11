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
#include "AssemblerWorker.h"

#include "ChunkStorage.h"
#include "control/FolderParams.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"
#include "folder/chunk/archive/Archive.h"
#include "folder/meta/Index.h"
#include "folder/meta/MetaStorage.h"
#include "util/file_util.h"
#include "util/log.h"
#include "util/readable.h"
#ifdef Q_OS_UNIX
#   include <sys/stat.h>
#endif

namespace librevault {

AssemblerWorker::AssemblerWorker(SignedMeta smeta, const FolderParams& params,
	                             MetaStorage* meta_storage,
	                             ChunkStorage* chunk_storage,
	                             PathNormalizer* path_normalizer,
	                             Archive* archive) :
	params_(params),
	meta_storage_(meta_storage),
	chunk_storage_(chunk_storage),
	path_normalizer_(path_normalizer),
	archive_(archive),
	smeta_(smeta),
	meta_(smeta.meta()) {}

AssemblerWorker::~AssemblerWorker() {}

blob AssemblerWorker::get_chunk_pt(const blob& ct_hash) const {
	LOGD("get_chunk_pt(" << ct_hash_readable(ct_hash) << ")");
	blob chunk = conv_bytearray(chunk_storage_->get_chunk(ct_hash));

	for(auto row : meta_storage_->index->db().exec("SELECT size, iv FROM chunk WHERE ct_hash=:ct_hash", {{":ct_hash", ct_hash}})) {
		return Meta::Chunk::decrypt(chunk, row[0].as_uint(), params_.secret.get_Encryption_Key(), row[1].as_blob());
	}
	throw ChunkStorage::no_such_chunk();
}

void AssemblerWorker::run() noexcept {
	LOGFUNC();

	try {
		bool assembled = false;
		switch(meta_.meta_type()) {
			case Meta::FILE: assembled = assemble_file();
				break;
			case Meta::DIRECTORY: assembled = assemble_directory();
				break;
			case Meta::SYMLINK: assembled = assemble_symlink();
				break;
			case Meta::DELETED: assembled = assemble_deleted();
				break;
			default: throw error(std::string("Unexpected meta type:") + std::to_string(meta_.meta_type()));
		}
		if(assembled) {
			if(meta_.meta_type() != Meta::DELETED)
				apply_attrib();

			meta_storage_->index->db().exec("UPDATE meta SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta_.path_id()}});
		}
	}catch(std::exception& e) {
		LOGW(BOOST_CURRENT_FUNCTION << " path:" << meta_.path(params_.secret).c_str() << " e:" << e.what()); // FIXME: Plaintext path in logs may violate user's privacy.
	}
}

bool AssemblerWorker::assemble_deleted() {
	LOGFUNC();

	fs::path file_path = path_normalizer_->absolute_path(meta_.path(params_.secret));
	QByteArray normpath = QByteArray::fromStdString(meta_.path(params_.secret));
	QString denormpath = path_normalizer_->denormalizePath(normpath);

	auto file_type = fs::symlink_status(file_path).type();

	// Suppress unnecessary events on dir_monitor.
	meta_storage_->prepareAssemble(normpath, Meta::DELETED);

	if(file_type == fs::directory_file) {
		if(fs::is_empty(file_path)) // Okay, just remove this empty directory
			fs::remove(file_path);
		else  // Oh, damn, this is very NOT RIGHT! So, we have DELETED directory with NOT DELETED files in it
			fs::remove_all(file_path);  // TODO: Okay, this is a horrible solution
	}

	if(file_type == fs::symlink_file || file_type == fs::file_not_found)
		fs::remove(file_path);
	else if(file_type == fs::regular_file)
		archive_->archive(denormpath);
	// TODO: else

	return true;    // Maybe, something else?
}

bool AssemblerWorker::assemble_symlink() {
	LOGFUNC();

	fs::path file_path = path_normalizer_->absolute_path(meta_.path(params_.secret));
	QByteArray normpath = QByteArray::fromStdString(meta_.path(params_.secret));
	QString denormpath = path_normalizer_->denormalizePath(normpath);

	fs::remove_all(file_path);
	fs::create_symlink(meta_.symlink_path(params_.secret), file_path);

	return true;    // Maybe, something else?
}

bool AssemblerWorker::assemble_directory() {
	LOGFUNC();

	QByteArray normpath = QByteArray::fromStdString(meta_.path(params_.secret));
	QString denormpath = path_normalizer_->denormalizePath(normpath);
	fs::path bdenormpath(denormpath.toStdString());

	bool create_new = true;
	if(fs::status(bdenormpath).type() != fs::file_type::directory_file)
		create_new = !fs::remove(bdenormpath);
	meta_storage_->prepareAssemble(normpath, Meta::DIRECTORY, create_new);

	if(create_new) fs::create_directories(bdenormpath);

	return true;    // Maybe, something else?
}

bool AssemblerWorker::assemble_file() {
	LOGFUNC();

	// Check if we have all needed chunks
	for(auto b : chunk_storage_->make_bitfield(meta_))
		if(!b) return false;    // retreat!

	//
	auto assembled_file = fs::path(params_.system_path.toStdWString()) / fs::unique_path("assemble-%%%%-%%%%-%%%%-%%%%");

	QByteArray normpath = QByteArray::fromStdString(meta_.path(params_.secret));
	QString denormpath = path_normalizer_->denormalizePath(normpath);
	fs::path bdenormpath(denormpath.toStdString());

	// TODO: Check for assembled chunk and try to extract them and push into encstorage.
	file_wrapper assembling_file(assembled_file, "wb"); // Opening file

	for(auto chunk : meta_.chunks()) {
		blob chunk_pt = get_chunk_pt(chunk.ct_hash);
		assembling_file.ios().write((const char*)chunk_pt.data(), chunk_pt.size());	// Writing to file
	}

	assembling_file.close();	// Closing file. Super!

	fs::last_write_time(assembled_file, meta_.mtime());

	meta_storage_->prepareAssemble(normpath, Meta::FILE, fs::exists(bdenormpath));

	archive_->archive(denormpath);
	fs::rename(assembled_file, bdenormpath);

	meta_storage_->index->db().exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta_.path_id()}});

	chunk_storage_->cleanup(meta_);

	return true;
}

void AssemblerWorker::apply_attrib() {
	fs::path file_path = path_normalizer_->absolute_path(meta_.path(params_.secret));

#if BOOST_OS_UNIX
	if(params_.preserve_unix_attrib) {
		if(meta_.meta_type() != Meta::SYMLINK) {
			int ec = 0;
			ec = chmod(file_path.c_str(), meta_.mode());
			if(ec) LOGW("Error applying mode to " << file_path.c_str());    // FIXME: full_path in logs may violate user's privacy.
			ec = chown(file_path.c_str(), meta_.uid(), meta_.gid());
			if(ec) LOGW("Error applying uid/gid to " << file_path.c_str()); // FIXME: full_path in logs may violate user's privacy.
		}
	}
#elif BOOST_OS_WINDOWS
	// Apply Windows attrib here.
#endif
}

} /* namespace librevault */
