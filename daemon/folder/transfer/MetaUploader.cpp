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
#include "MetaUploader.h"
#include "folder/storage/ChunkStorage.h"
#include "folder/storage/Storage.h"
#include "folder/storage/Index.h"
#include "p2p/Peer.h"
#include "p2p/MessageHandler.h"

namespace librevault {

MetaUploader::MetaUploader(Storage* meta_storage, ChunkStorage* chunk_storage, QObject* parent) :
	QObject(parent),
	storage_(meta_storage), chunk_storage_(chunk_storage) {
	LOGFUNC();
}

void MetaUploader::broadcast_meta(QList<Peer*> remotes, const MetaInfo::PathRevision& revision, QBitArray bitfield) {
	for(auto remote : remotes) {
		remote->messageHandler()->sendHaveMeta(revision, bitfield);
	}
}

void MetaUploader::handle_handshake(Peer* remote) {
	for(auto& meta : storage_->index()->getMeta()) {
		remote->messageHandler()->sendHaveMeta(meta.metaInfo().path_revision(), chunk_storage_->make_bitfield(meta.metaInfo()));
	}
}

void MetaUploader::handle_meta_request(Peer* remote, const MetaInfo::PathRevision& revision) {
	try {
		remote->messageHandler()->sendMetaReply(storage_->index()->getMeta(revision), chunk_storage_->make_bitfield(
            storage_->index()->getMeta(revision).metaInfo()));
	}catch(Index::no_such_meta& e){
		LOGW("Requested nonexistent Meta");
	}
}

} /* namespace librevault */
