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
#include "MetaDownloader.h"
#include "Downloader.h"
#include "folder/FolderGroup.h"
#include "p2p/Peer.h"
#include "p2p/MessageHandler.h"
#include "folder/meta/MetaStorage.h"

namespace librevault {

MetaDownloader::MetaDownloader(const FolderParams& params, MetaStorage* meta_storage, Downloader* downloader, QObject* parent) :
	QObject(parent),
	params_(params),
	meta_storage_(meta_storage),
	downloader_(downloader) {
	LOGFUNC();
}

void MetaDownloader::handle_have_meta(Peer* origin, const Meta::PathRevision& revision, QBitArray bitfield) {
	if(meta_storage_->haveMeta(revision))
		downloader_->notifyRemoteMeta(origin, revision, bitfield);
	else if(meta_storage_->putAllowed(revision))
		origin->messageHandler()->sendMetaRequest(revision);
	else
		LOGD("Remote node notified us about an expired Meta");
}

void MetaDownloader::handle_meta_reply(Peer* origin, const SignedMeta& smeta, QBitArray bitfield) {
	if(smeta.isValid(params_.secret), meta_storage_->putAllowed(smeta.meta().path_revision())) {
		meta_storage_->putMeta(smeta);
		downloader_->notifyRemoteMeta(origin, smeta.meta().path_revision(), bitfield);
	}else
		LOGD("Remote node posted to us about an expired Meta");
}

} /* namespace librevault */
