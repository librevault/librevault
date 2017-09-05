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
#include <PathNormalizer.h>
#include <ChunkInfo.h>
#include "folder/meta/MetaStorage.h"
#include "util/conv_fspath.h"
#include "util/log.h"
#include <boost/filesystem.hpp>
#include <QBitArray>
#include <QDir>
#include <QLoggingCategory>
#include <QSaveFile>
#ifdef Q_OS_UNIX
#   include <sys/stat.h>
#endif
#ifdef Q_OS_WIN
#   include <windows.h>
#endif

Q_LOGGING_CATEGORY(log_assembler, "folder.chunk.assembler")

namespace librevault {

AssemblerWorker::AssemblerWorker(SignedMeta smeta, const FolderParams& params,
	                             MetaStorage* meta_storage,
	                             ChunkStorage* chunk_storage,
	                             Archive* archive) :
	params_(params),
	meta_storage_(meta_storage),
	chunk_storage_(chunk_storage),
	archive_(archive),
	smeta_(smeta),
	meta_(smeta.metaInfo()) {}

AssemblerWorker::~AssemblerWorker() {}

QByteArray AssemblerWorker::get_chunk_pt(QByteArray ct_hash) const {
	try {
		QPair<quint32, QByteArray> size_iv = meta_storage_->getChunkSizeIv(ct_hash);
		return ChunkInfo::decrypt(chunk_storage_->get_chunk(ct_hash), size_iv.first, params_.secret.encryptionKey(), size_iv.second);
	}catch(std::exception& e){
		qCWarning(log_assembler) << "Could not get plaintext chunk (which is marked as existing in index), DB collision";
		throw ChunkStorage::no_such_chunk();
	}
}

void AssemblerWorker::run() noexcept {
	LOGFUNC();

	normpath_ = meta_.path().plaintext(params_.secret.encryptionKey());
	denormpath_ = PathNormalizer::absolutizePath(normpath_, params_.path);

	try {
		bool assembled = false;
		switch(meta_.kind()) {
			case MetaInfo::FILE: assembled = assemble_file();
				break;
			case MetaInfo::DIRECTORY: assembled = assemble_directory();
				break;
			case MetaInfo::SYMLINK: assembled = assemble_symlink();
				break;
			case MetaInfo::DELETED: assembled = assemble_deleted();
				break;
			default:
				qWarning() << QString("Unexpected meta type: %1").arg(meta_.kind());
				throw abort_assembly();
		}
		if(assembled) {
			if(meta_.kind() != MetaInfo::DELETED)
				apply_attrib();

			meta_storage_->markAssembled(meta_.pathKeyedHash());
			chunk_storage_->cleanup(meta_);
		}
	}catch(abort_assembly& e) {  // Already handled
	}catch(std::exception& e) {
		qCWarning(log_assembler) << "Unknown exception while assembling:" << meta_.path().plaintext(params_.secret.encryptionKey()) << "E:" << e.what();    // FIXME: #83
	}
}

bool AssemblerWorker::assemble_deleted() {
	LOGFUNC();
	return QFile::remove(denormpath_);
}

bool AssemblerWorker::assemble_symlink() {
	LOGFUNC();

	boost::filesystem::path denormpath_fs(denormpath_.toStdWString());

	boost::filesystem::remove_all(denormpath_fs);
	boost::filesystem::create_symlink(meta_.symlinkTarget().plaintext(params_.secret.encryptionKey()).toStdString(), denormpath_fs);

	return true;    // Maybe, something else?
}

bool AssemblerWorker::assemble_directory() {
	LOGFUNC();

	boost::filesystem::path denormpath_fs(denormpath_.toStdWString());

	bool create_new = true;
	if(boost::filesystem::status(denormpath_fs).type() != boost::filesystem::file_type::directory_file)
		create_new = !boost::filesystem::remove(denormpath_fs);
	meta_storage_->prepareAssemble(normpath_, MetaInfo::DIRECTORY, create_new);

	if(create_new)
		QDir().mkpath(denormpath_);

	return true;    // Maybe, something else?
}

bool AssemblerWorker::assemble_file() {
	LOGFUNC();

	// Check if we have all needed chunks
	auto bitfield = chunk_storage_->make_bitfield(meta_);
	if(bitfield.count(true) != bitfield.size()) return false;

	//
	QString assembly_path = params_.system_path + "/" + conv_fspath(boost::filesystem::unique_path("assemble-%%%%-%%%%-%%%%-%%%%"));

	// TODO: Check for assembled chunk and try to extract them and push into encstorage.
	QSaveFile assembly_f(assembly_path); // Opening file
	if(! assembly_f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		qCWarning(log_assembler) << "File cannot be opened:" << assembly_path << "E:" << assembly_f.errorString();  // FIXME: #83
		throw abort_assembly();
	}

	for(auto chunk : meta_.chunks()) {
		assembly_f.write(get_chunk_pt(chunk.ctHash())); // Writing to file
	}

	if(!assembly_f.commit()) {
		qCWarning(log_assembler) << "File cannot be written:" << assembly_path << "E:" << assembly_f.errorString(); // FIXME: #83
		throw abort_assembly();
	}

	{
		boost::system::error_code ec;
		boost::filesystem::last_write_time(conv_fspath(assembly_path), meta_.mtime(), ec);
		if(ec) {
			qCWarning(log_assembler) << "Could not set mtime on file:" << assembly_path << "E:" << QString::fromStdString(ec.message());    // FIXME: #83
		}
	}

	meta_storage_->prepareAssemble(normpath_, MetaInfo::FILE, boost::filesystem::exists(conv_fspath(denormpath_)));

	if(! QFile::remove(denormpath_)) {
		qCWarning(log_assembler) << "Item cannot be archived/removed:" << denormpath_;  // FIXME: #83
		throw abort_assembly();
	}
	if(! QFile::rename(assembly_path, denormpath_)) {
		qCWarning(log_assembler) << "File cannot be moved to its final location:" << denormpath_ << "Current location:" << assembly_path;   // FIXME: #83
		throw abort_assembly();
	}

	return true;
}

void AssemblerWorker::apply_attrib() {
#if defined(Q_OS_UNIX)
	if(params_.preserve_unix_attrib) {
		if(meta_.kind() != MetaInfo::SYMLINK) {
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
