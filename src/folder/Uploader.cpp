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
	log_->trace() << log_tag() << "Uploader()";
}

void Uploader::handle_interested(std::shared_ptr<RemoteDirectory> remote) {
	// TODO: write good choking algorithm.
	remote->unchoke();
}
void Uploader::handle_not_interested(std::shared_ptr<RemoteDirectory> remote) {
	// TODO: write good choking algorithm.
	remote->choke();
}

void Uploader::request_chunk(std::shared_ptr<RemoteDirectory> origin, const blob& encrypted_data_hash, uint32_t offset, uint32_t size) {
	try {
		auto chunk = exchange_group_.fs_dir()->get_chunk(encrypted_data_hash, offset, size);
		origin->post_chunk(encrypted_data_hash, offset, chunk);
	}catch(AbstractFolder::no_such_block& e){
		log_->warn() << log_tag() << "Requested nonexistent chunk";
	}
}

} /* namespace librevault */
