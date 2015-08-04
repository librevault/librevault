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
#pragma once
#include "../../pch.h"
#include "../Key.h"
#include "Index.h"
#include "EncStorage.h"
#include "OpenStorage.h"

namespace librevault {

class FSDirectory;
class Indexer {
public:
	Indexer(FSDirectory& dir, Session& session);
	virtual ~Indexer();

	// Index manipulation
	void index(const std::string& file_path);
	void index(const std::set<std::string>& file_path);

	void async_index(const std::string& file_path, std::function<void(AbstractDirectory::SignedMeta)> callback);
	void async_index(const std::set<std::string>& file_path, std::function<void(AbstractDirectory::SignedMeta)> callback);

	// Meta functions
	blob make_Meta(const std::string& relpath);

	AbstractDirectory::SignedMeta sign(const blob& meta) const;

private:
	std::shared_ptr<spdlog::logger> log_;
	FSDirectory& dir_;

	const Key& key_;
	Index& index_;
	EncStorage& enc_storage_;
	OpenStorage& open_storage_;

	Session& session_;
};

} /* namespace librevault */
