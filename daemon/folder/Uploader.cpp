/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
 */
#include "Uploader.h"

#include "FolderGroup.h"

#include "src/folder/fs/FSFolder.h"
#include "src/folder/p2p/P2PFolder.h"

#include "../Client.h"

namespace librevault {

Uploader::Uploader(Client& client, FolderGroup& exchange_group) :
		Loggable(client, "Uploader"),
		client_(client),
		exchange_group_(exchange_group) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
}

void Uploader::handle_interested(std::shared_ptr<RemoteFolder> remote) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	// TODO: write good choking algorithm.
	remote->unchoke();
}
void Uploader::handle_not_interested(std::shared_ptr<RemoteFolder> remote) {
	log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;

	// TODO: write good choking algorithm.
	remote->choke();
}

void Uploader::request_block(std::shared_ptr<RemoteFolder> origin, const blob& ct_hash, uint32_t offset, uint32_t size) {
	try {
		origin->post_block(ct_hash, offset, get_block(ct_hash, offset, size));
	}catch(AbstractFolder::no_such_chunk& e){
		log_->warn() << log_tag() << "Requested nonexistent block";
	}
}

blob Uploader::get_block(const blob& ct_hash, uint32_t offset, uint32_t size) {
	auto chunk = exchange_group_.fs_dir()->get_chunk(ct_hash);
	if(offset < chunk.size() && size <= chunk.size()-offset)
		return blob(chunk.begin()+offset, chunk.begin()+offset+size);
	else
		throw AbstractFolder::no_such_chunk();
}

} /* namespace librevault */
