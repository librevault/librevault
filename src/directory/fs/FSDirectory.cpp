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
#include "FSProvider.h"

#include "../../../contrib/crypto/Base32.h"

#include "../../Session.h"
#include "../../net/parse_url.h"

namespace librevault {

FSDirectory::FSDirectory(ptree dir_options, Session& session, FSProvider& provider) :
		log_(spdlog::get("Librevault")),
		dir_options_(std::move(dir_options)),
		key_(dir_options_.get<std::string>("key")),

		open_path_(dir_options_.get<fs::path>("open_path")),
		block_path_(dir_options_.get<fs::path>("block_path", open_path_ / ".librevault")),
		db_path_(dir_options_.get<fs::path>("db_path", block_path_ / "directory.db")),
		asm_path_(dir_options_.get<fs::path>("asm_path", block_path_ / "assembled.part")) {
	log_->debug() << "New FSDirectory: Key type=" << (char)key_.get_type();

	index = std::make_unique<Index>(*this, session);
	enc_storage = std::make_unique<EncStorage>(*this, session);
	open_storage = std::make_unique<OpenStorage>(*this, session);
	indexer = std::make_unique<Indexer>(*this, session);
	auto_indexer = std::make_unique<AutoIndexer>(*this, session, std::bind(&FSDirectory::handle_smeta, this, std::placeholders::_1));
}

FSDirectory::~FSDirectory() {}

void FSDirectory::handle_smeta(AbstractDirectory::SignedMeta smeta) {
	Meta m; m.ParseFromArray(smeta.meta.data(), smeta.meta.size());
	blob path_hmac(m.path_hmac().begin(), m.path_hmac().end());
	announce_revision(path_hmac, m.mtime());
}

void FSDirectory::announce_revision(const blob& path_hmac, int64_t revision){
	log_->debug() << "Created revision " << revision << " of " << (std::string)crypto::Base32().to(path_hmac);
}

} /* namespace librevault */
