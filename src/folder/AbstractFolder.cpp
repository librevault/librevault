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
#include "src/Client.h"
#include "AbstractFolder.h"

namespace librevault {

AbstractFolder::AbstractFolder(Client& client) :
	Loggable(client), client_(client) {}
AbstractFolder::~AbstractFolder() {}

std::shared_ptr<FolderGroup> AbstractFolder::folder_group() {
	auto group_ptr = folder_group_.lock();
	if(group_ptr)
		return group_ptr;
	else {
		log_->error() << log_tag() << "Folder group is not available (Application error)";
		throw error("Folder group is not available (Application error)");
	}
}

std::string AbstractFolder::path_id_readable(const blob& path_id) {
	return crypto::Base32().to_string(path_id);
}

std::string AbstractFolder::ct_hash_readable(const blob& ct_hash) {
	return crypto::Base32().to_string(ct_hash);
}

} /* namespace librevault */
