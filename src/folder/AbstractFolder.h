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
#pragma once
#include <librevault/Meta.h>
#include <librevault/util/bitfield_convert.h>
#include "src/util/Loggable.h"

namespace librevault {

class Client;
class FolderGroup;

class AbstractFolder : public Loggable {
	friend class FolderGroup;
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("Directory error") {}
	};

	struct no_such_meta : public error {
		no_such_meta() : error("Requested Meta not found"){}
	};

	struct no_such_block : public error {
		no_such_block() : error("Requested Block not found"){}
	};

	AbstractFolder(Client& client);
	virtual ~AbstractFolder();

	// AbstractFolder properties
	virtual std::string name() const = 0;
	std::shared_ptr<FolderGroup> folder_group();

	// Other functions
	static std::string path_id_readable(const blob& path_id);
	static std::string encrypted_data_hash_readable(const blob& block_id);

protected:
	Client& client_;

	std::weak_ptr<FolderGroup> folder_group_;
};

} /* namespace librevault */
