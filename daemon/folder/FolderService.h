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
#include "util/blob.h"
#include "util/log_scope.h"
#include "util/multi_io_service.h"
#include "util/scoped_async_queue.h"
#include <boost/signals2/signal.hpp>
#include <json/json.h>

namespace librevault {

/* Folder info */
class FolderGroup;
class FolderParams;
class Secret;

class FolderService {
	LOG_SCOPE("FolderService");
public:
	struct samekey_error : std::runtime_error {
		samekey_error() : std::runtime_error("Multiple directories with the same key (or derived from the same key) are not supported now") {}
	};

	FolderService();
	virtual ~FolderService();

	void run();
	void stop();

	/* Signals */
	boost::signals2::signal<void(std::shared_ptr<FolderGroup>)> folder_added_signal;
	boost::signals2::signal<void(std::shared_ptr<FolderGroup>)> folder_removed_signal;

	/* FolderGroup nanagenent */
	void add_folder(Json::Value json_folder);    // Adds folder into config, so JSON. Also, invokes init_folder.
	void remove_folder(const blob& folder_hash);   // Invokes deinit_folder and removes folder from config.

	void init_folder(const FolderParams& params);
	void deinit_folder(const blob& folder_hash);

	std::shared_ptr<FolderGroup> get_group(const blob& hash);
	std::vector<std::shared_ptr<FolderGroup>> groups() const;

private:
	multi_io_service bulk_ios_;
	multi_io_service serial_ios_;
	std::map<blob, std::shared_ptr<FolderGroup>> hash_group_;
	ScopedAsyncQueue init_queue_;
};

} /* namespace librevault */
