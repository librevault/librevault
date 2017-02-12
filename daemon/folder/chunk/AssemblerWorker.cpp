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
#include "util/readable.h"
#include <QDir>
#include <QLoggingCategory>
#include <QSaveFile>
#ifdef Q_OS_UNIX
#   include <sys/stat.h>
#endif

Q_LOGGING_CATEGORY(log_assembler, "folder.chunk.assembler")

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

QByteArray AssemblerWorker::get_chunk_pt(const blob& ct_hash) const {
	blob chunk = conv_bytearray(chunk_storage_->get_chunk(ct_hash));

	for(auto row : meta_storage_->index->db().exec("SELECT size, iv FROM chunk WHERE ct_hash=:ct_hash", {{":ct_hash", ct_hash}})) {
		blob chunk_pt_v = Meta::Chunk::decrypt(chunk, row[0].as_uint(), params_.secret.get_Encryption_Key(), row[1].as_blob());
		return QByteArray::fromRawData((const char*)chunk_pt_v.data(), chunk_pt_v.size());
	}
	qCWarning(log_assembler) << "Could not get plaintext chunk (which is marked as existing in index), DB collision";
	throw ChunkStorage::no_such_chunk();
}

void AssemblerWorker::run() noexcept {
	LOGFUNC();

	normpath_ = QByteArray::fromStdString(meta_.path(params_.secret));
	denormpath_ = path_normalizer_->denormalizePath(normpath_);

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
			default:
				qWarning() << QString("Unexpected meta type: %1").arg(meta_.meta_type());
				throw abort_assembly();
		}
		if(assembled) {
			if(meta_.meta_type() != Meta::DELETED)
				apply_attrib();

			meta_storage_->index->db().exec("UPDATE meta SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta_.path_id()}});
		}
	}catch(abort_assembly& e) {  // Already handled
	}catch(std::exception& e) {
		qCWarning(log_assembler) << "Unknown exception while assembling:" << meta_.path(params_.secret).c_str() << "E:" << e.what();    // FIXME: #83
	}
}

bool AssemblerWorker::assemble_deleted() {
	LOGFUNC();
	fs::path denormpath_fs(denormpath_.toStdWString());

	auto file_type = fs::symlink_status(denormpath_fs).type();

	// Suppress unnecessary events on dir_monitor.
	meta_storage_->prepareAssemble(normpath_, Meta::DELETED);

	if(file_type == fs::directory_file) {
		if(fs::is_empty(denormpath_fs)) // Okay, just remove this empty directory
			fs::remove(denormpath_fs);
		else  // Oh, damn, this is very NOT RIGHT! So, we have DELETED directory with NOT DELETED files in it
			fs::remove_all(denormpath_fs);  // TODO: Okay, this is a horrible solution
	}

	if(file_type == fs::symlink_file || file_type == fs::file_not_found)
		fs::remove(denormpath_fs);
	else if(file_type == fs::regular_file)
		archive_->archive(denormpath_);
	// TODO: else

	return true;    // Maybe, something else?
}

bool AssemblerWorker::assemble_symlink() {
	LOGFUNC();

	fs::path denormpath_fs(denormpath_.toStdWString());

	fs::remove_all(denormpath_fs);
	fs::create_symlink(meta_.symlink_path(params_.secret), denormpath_fs);

	return true;    // Maybe, something else?
}

bool AssemblerWorker::assemble_directory() {
	LOGFUNC();

	fs::path denormpath_fs(denormpath_.toStdWString());

	bool create_new = true;
	if(fs::status(denormpath_fs).type() != fs::file_type::directory_file)
		create_new = !fs::remove(denormpath_fs);
	meta_storage_->prepareAssemble(normpath_, Meta::DIRECTORY, create_new);

	if(create_new)
		QDir().mkpath(denormpath_);

	return true;    // Maybe, something else?
}

bool AssemblerWorker::assemble_file() {
	LOGFUNC();

	// Check if we have all needed chunks
	for(auto b : chunk_storage_->make_bitfield(meta_))
		if(!b) return false;    // retreat!

	//
	QString assembly_path = params_.system_path + "/" + QString::fromStdWString(fs::unique_path("assemble-%%%%-%%%%-%%%%-%%%%").wstring());

	// TODO: Check for assembled chunk and try to extract them and push into encstorage.
	QSaveFile assembly_f(assembly_path); // Opening file
	if(! assembly_f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		qCWarning(log_assembler) << "File cannot be opened:" << assembly_path << "E:" << assembly_f.errorString();  // FIXME: #83
		throw abort_assembly();
	}

	for(auto chunk : meta_.chunks()) {
		assembly_f.write(get_chunk_pt(chunk.ct_hash)); // Writing to file
	}

	if(!assembly_f.commit()) {
		qCWarning(log_assembler) << "File cannot be written:" << assembly_path << "E:" << assembly_f.errorString(); // FIXME: #83
		throw abort_assembly();
	}

	{
		boost::system::error_code ec;
		fs::last_write_time(fs::path(assembly_path.toStdWString()), meta_.mtime(), ec);
		if(ec) {
			qCWarning(log_assembler) << "Could not set mtime on file:" << assembly_path << "E:" << QString::fromStdString(ec.message());    // FIXME: #83
		}
	}

	meta_storage_->prepareAssemble(normpath_, Meta::FILE, fs::exists(fs::path(denormpath_.toStdWString())));

	if(! archive_->archive(denormpath_)) {
		qCWarning(log_assembler) << "Item cannot be archived/removed:" << denormpath_;  // FIXME: #83
		throw abort_assembly();
	}
	if(! QFile::rename(assembly_path, denormpath_)) {
		qCWarning(log_assembler) << "File cannot be moved to its final location:" << denormpath_ << "Current location:" << assembly_path;   // FIXME: #83
		throw abort_assembly();
	}

	meta_storage_->index->db().exec("UPDATE openfs SET assembled=1 WHERE path_id=:path_id", {{":path_id", meta_.path_id()}});

	chunk_storage_->cleanup(meta_);

	return true;
}

void AssemblerWorker::apply_attrib() {
#if defined(Q_OS_UNIX)
	if(params_.preserve_unix_attrib) {
		if(meta_.meta_type() != Meta::SYMLINK) {
			int ec = 0;
			ec = chmod(QFile::encodeName(denormpath_), meta_.mode());
			if(ec) {
				qCWarning(log_assembler) << "Error applying mode to" << denormpath_;    // FIXME: #83
			}

			ec = chown(QFile::encodeName(denormpath_), meta_.uid(), meta_.gid());
			if(ec) {
				qCWarning(log_assembler) << "Error applying uid/gid to" << denormpath_;
			}
		}
	}
#elif defined(Q_OS_WIN)
	// Apply Windows attrib here.
#endif
}

} /* namespace librevault */
