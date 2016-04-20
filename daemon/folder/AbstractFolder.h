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
#include <librevault/util/bitfield_convert.h>
#include "util/Loggable.h"
#include "pch.h"

namespace librevault {

class Client;
class FolderGroup;

class AbstractFolder : public Loggable {
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("Directory error") {}
	};

	struct no_such_meta : public error {
		no_such_meta() : error("Requested Meta not found"){}
	};

	struct no_such_chunk : public error {
		no_such_chunk() : error("Requested Chunk not found"){}
	};

	AbstractFolder(Client& client);
	virtual ~AbstractFolder();

	// AbstractFolder properties
	const std::string& name() const {return name_;}

	// Other functions
	static std::string path_id_readable(const blob& path_id);
	static std::string ct_hash_readable(const blob& ct_hash);

protected:
	Client& client_;
};

} /* namespace librevault */
