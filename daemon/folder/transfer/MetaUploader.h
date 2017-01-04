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
#include "util/log_scope.h"
#include <librevault/Meta.h>
#include <librevault/util/bitfield_convert.h>
#include <QObject>
#include <set>

namespace librevault {

class RemoteFolder;
class MetaStorage;
class ChunkStorage;

class MetaUploader : public QObject {
	Q_OBJECT
	LOG_SCOPE("MetaUploader");
public:
	MetaUploader(MetaStorage& meta_storage, ChunkStorage& chunk_storage);

	void broadcast_meta(std::set<RemoteFolder*> remotes, const Meta::PathRevision& revision, const bitfield_type& bitfield);

	/* Message handlers */
	void handle_handshake(RemoteFolder* remote);
	void handle_meta_request(RemoteFolder* remote, const Meta::PathRevision& revision);

private:
	MetaStorage& meta_storage_;
	ChunkStorage& chunk_storage_;
};

} /* namespace librevault */
