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
#include "util/blob.h"
#include "util/log_scope.h"
#include <librevault/SignedMeta.h>
#include <QString>
#include <QRunnable>
#include <QObject>
#include <map>

namespace librevault {

class FolderParams;
class Index;
class IgnoreList;
class PathNormalizer;
class IndexerWorker : public QObject, public QRunnable {
	Q_OBJECT
	LOG_SCOPE("Indexer");
signals:
	void metaCreated(SignedMeta smeta);

public:
	struct error : std::runtime_error {
		error(QString what) : std::runtime_error(what.toStdString()) {}
		error() : error("Indexer error") {}
	};

	struct abort_index : error {
		abort_index(QString what) : error(what) {}
	};

	struct unsupported_filetype : abort_index {
		unsupported_filetype() : abort_index("File type is unsuitable for indexing. Only Files, Directories and Symbolic links are supported") {}
	};

	IndexerWorker(QString abspath, const FolderParams& params, Index& index, IgnoreList& ignore_list, PathNormalizer& path_normalizer);
	virtual ~IndexerWorker();

	// Index manipulation
	void run() noexcept override;

private:
	QString abspath_;
	const FolderParams& params_;
	Index& index_;
	IgnoreList& ignore_list_;
	PathNormalizer& path_normalizer_;

	const Secret& secret_;

	Meta new_meta_;
	SignedMeta old_smeta_, new_smeta_;

	/* Status */
	bool active = true;

	void make_Meta();

	/* File analyzers */
	Meta::Type get_type();
	void update_fsattrib();
	void update_chunks();
	Meta::Chunk populate_chunk(const blob& data, const std::map<blob, blob>& pt_hmac__iv);
};

} /* namespace librevault */
