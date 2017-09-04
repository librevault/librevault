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
#include "SignedMeta.h"
#include <QObject>

namespace librevault {

class DirectoryPoller;
class DirectoryWatcher;
class FolderParams;
class IgnoreList;
class Index;
class IndexerQueue;

class MetaStorage : public QObject {
	Q_OBJECT
signals:
	void metaAdded(SignedMeta meta);
	void metaAddedExternal(SignedMeta meta);

public:
	struct no_such_meta : public std::runtime_error {
		no_such_meta() : std::runtime_error("Requested Meta not found"){}
	};

	MetaStorage(const FolderParams& params, IgnoreList* ignore_list, QObject* parent);
	virtual ~MetaStorage();

	bool haveMeta(const MetaInfo::PathRevision& path_revision) noexcept;
	SignedMeta getMeta(const MetaInfo::PathRevision& path_revision);
	SignedMeta getMeta(QByteArray path_id);
	QList<SignedMeta> getMeta();
	QList<SignedMeta> getExistingMeta();
	QList<SignedMeta> getIncompleteMeta();
	void putMeta(const SignedMeta& signed_meta, bool fully_assembled = false);
	QList<SignedMeta> containingChunk(QByteArray ct_hash);
	QPair<quint32, QByteArray> getChunkSizeIv(QByteArray ct_hash);

	// Assembled index
	void markAssembled(QByteArray path_id);
	bool isChunkAssembled(QByteArray ct_hash);

	bool putAllowed(const MetaInfo::PathRevision& path_revision) noexcept;

	void prepareAssemble(QByteArray normpath, MetaInfo::Kind type, bool with_removal = false);

private:
	Index* index_;
	IndexerQueue* indexer_;
	DirectoryPoller* poller_;
	DirectoryWatcher* watcher_;
};

} /* namespace librevault */
