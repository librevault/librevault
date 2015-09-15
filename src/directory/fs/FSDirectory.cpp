/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "FSDirectory.h"

#include "../../Session.h"
#include "../../net/parse_url.h"
#include "../Exchanger.h"
#include "../p2p/P2PDirectory.h"

namespace librevault {

FSDirectory::FSDirectory(ptree dir_options, Session& session, Exchanger& exchanger) :
		AbstractDirectory(session, exchanger),
		dir_options_(std::move(dir_options)),
		key_(dir_options_.get<std::string>("key")),

		open_path_(dir_options_.get<fs::path>("open_path")),
		block_path_(dir_options_.get<fs::path>("block_path", open_path_ / ".librevault")),
		db_path_(dir_options_.get<fs::path>("db_path", block_path_ / "directory.db")),
		asm_path_(dir_options_.get<fs::path>("asm_path", block_path_ / "assembled.part")) {
	log_->debug() << "New FSDirectory: Key type=" << (char)key_.get_type();

	index = std::make_unique<Index>(*this, session);
	if(key_.get_type() <= Key::Type::Download){
		enc_storage = std::make_unique<EncStorage>(*this, session_);
	}
	if(key_.get_type() <= Key::Type::ReadOnly){
		open_storage = std::make_unique<OpenStorage>(*this, session_);
	}
	if(key_.get_type() <= Key::Type::ReadWrite){
		indexer = std::make_unique<Indexer>(*this, session_);
		auto_indexer = std::make_unique<AutoIndexer>(*this, session_, std::bind(&FSDirectory::handle_smeta, this, std::placeholders::_1));
	}
}

FSDirectory::~FSDirectory() {}

void FSDirectory::attach_remote(std::shared_ptr<P2PDirectory> remote_ptr) {
	remotes.insert(remote_ptr);
	log_->debug() << "Attached remote " << remote_ptr->remote_string() << " to " << name();
}

void FSDirectory::detach_remote(std::shared_ptr<P2PDirectory> remote_ptr) {
	remotes.erase(remote_ptr);
	log_->debug() << "Detached remote " << remote_ptr->remote_string() << " from " << name();
}

void FSDirectory::handle_smeta(AbstractDirectory::SignedMeta smeta) {
	Meta m; m.ParseFromArray(smeta.meta.data(), smeta.meta.size());
	blob path_hmac(m.path_hmac().begin(), m.path_hmac().end());

	log_->debug() << "Created revision " << m.mtime() << " of " << (std::string)crypto::Base32().to(path_hmac);
}

std::string FSDirectory::name() const {
	return open_path_.empty() ? block_path_.string() : open_path_.string();
}

} /* namespace librevault */
