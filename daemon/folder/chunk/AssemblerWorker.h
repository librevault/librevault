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
#pragma once
#include "blob.h"
#include "SignedMeta.h"
#include <QObject>
#include <QRunnable>

namespace librevault {

class PathNormalizer;

class Archive;
class MetaStorage;
class FolderParams;
class ChunkStorage;
class Secret;

class AssemblerWorker : public QObject, public QRunnable {
public:
	struct abort_assembly : std::runtime_error {
		explicit abort_assembly() : std::runtime_error("Assembly aborted") {}
	};

	AssemblerWorker(SignedMeta smeta,
	                const FolderParams& params,
					MetaStorage* meta_storage,
					ChunkStorage* chunk_storage,
					PathNormalizer* path_normalizer,
					Archive* archive);
	virtual ~AssemblerWorker();

	void run() noexcept override;

private:
	const FolderParams& params_;
	MetaStorage* meta_storage_;
	ChunkStorage* chunk_storage_;
	PathNormalizer* path_normalizer_;
	Archive* archive_;

	SignedMeta smeta_;
	const Meta& meta_;

	QByteArray normpath_;
	QString denormpath_;

	bool assemble_deleted();
	bool assemble_symlink();
	bool assemble_directory();
	bool assemble_file();

	void apply_attrib();

	QByteArray get_chunk_pt(QByteArray ct_hash) const;
};

} /* namespace librevault */
