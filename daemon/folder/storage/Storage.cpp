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
#include "Storage.h"
#include "scanner/DirectoryPoller.h"
#include "scanner/DirectoryWatcher.h"
#include "Index.h"
#include "ChunkStorage.h"
#include "scanner/IndexerQueue.h"
#include "control/FolderParams.h"
#include "assembler/AssemblerQueue.h"

namespace librevault {

Storage::Storage(const FolderParams& params, IgnoreList* ignore_list, QObject* parent) : QObject(parent) {
	index_ = new Index(params, this);
	indexer_ = new IndexerQueue(params, ignore_list, this);
	poller_ = new DirectoryPoller(params, ignore_list, this);
	watcher_ = new DirectoryWatcher(params, ignore_list, this);

	if(params.secret.canEncrypt()){
		connect(poller_, &DirectoryPoller::newPath, indexer_, &IndexerQueue::addIndexing);
		connect(watcher_, &DirectoryWatcher::newPath, indexer_, &IndexerQueue::addIndexing);

		poller_->setEnabled(true);
	}

	if(params.secret.canDecrypt()) {
		file_assembler = new AssemblerQueue(params, this, this);
		connect(index_, &Index::metaAddedExternal, file_assembler, &AssemblerQueue::addAssemble);
	}

	chunk_storage_ = new ChunkStorage(params, this, this);
}

void Storage::addDownloadedMetaInfo(const SignedMeta &signed_meta) {
	return index_->putMeta(signed_meta);	//reschedule
}

void Storage::handleAddedChunk(const QByteArray& ct_hash) {
	for(auto& smeta : index_->containingChunk(ct_hash))
		file_assembler->addAssemble(smeta);
}

} /* namespace librevault */
