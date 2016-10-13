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

#include "../FolderGroup.h"

#include "folder/fs/FSFolder.h"
#include "folder/fs/Index.h"
#include "../RemoteFolder.h"

#include "util/log.h"

namespace librevault {

MetaUploader::MetaUploader(FolderGroup& exchange_group) :
		exchange_group_(exchange_group) {
	LOGFUNC();
}

void MetaUploader::handle_handshake(std::shared_ptr<RemoteFolder> remote) {
	for(auto& meta : exchange_group_.fs_dir()->index->get_meta()) {
		remote->post_have_meta(meta.meta().path_revision(), exchange_group_.fs_dir()->get_bitfield(meta.meta().path_revision()));
	}
}

void MetaUploader::handle_meta_request(std::shared_ptr<RemoteFolder> origin, const Meta::PathRevision& revision) {
	try {
		origin->post_meta(exchange_group_.fs_dir()->get_meta(revision), exchange_group_.fs_dir()->get_bitfield(revision));
	}catch(AbstractFolder::no_such_meta& e){
		LOGW("Requested nonexistent Meta");
	}
}

} /* namespace librevault */
