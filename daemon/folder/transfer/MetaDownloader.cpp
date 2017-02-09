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
#include "folder/RemoteFolder.h"
#include "folder/meta/Index.h"
#include "folder/meta/MetaStorage.h"

namespace librevault {

MetaDownloader::MetaDownloader(MetaStorage* meta_storage, Downloader* downloader, QObject* parent) :
	QObject(parent),
	meta_storage_(meta_storage),
	downloader_(downloader) {
	LOGFUNC();
}

void MetaDownloader::handle_have_meta(RemoteFolder* origin, const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	if(meta_storage_->index->have_meta(revision))
		downloader_->notify_remote_meta(origin, revision, bitfield);
	else if(meta_storage_->index->put_allowed(revision))
		origin->request_meta(revision);
	else
		LOGD("Remote node notified us about an expired Meta");
}

void MetaDownloader::handle_meta_reply(RemoteFolder* origin, const SignedMeta& smeta, const bitfield_type& bitfield) {
	if(meta_storage_->index->put_allowed(smeta.meta().path_revision())) {
		meta_storage_->index->put_meta(smeta);
		downloader_->notify_remote_meta(origin, smeta.meta().path_revision(), bitfield);
	}else
		LOGD("Remote node posted to us about an expired Meta");
}

} /* namespace librevault */
