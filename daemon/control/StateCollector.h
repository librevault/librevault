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
#include <json/json.h>
#include <boost/signals2/signal.hpp>

namespace librevault {

class StateCollector {
	LOG_SCOPE("StateCollector");
public:
	StateCollector();
	~StateCollector();

	boost::signals2::signal<void(std::string, Json::Value)> global_state_changed;
	void global_state_set(const std::string& key, Json::Value value);

	boost::signals2::signal<void(const blob& folderid, std::string, Json::Value)> folder_state_changed;
	void folder_state_set(const blob& folderid, const std::string& key, Json::Value value);

	boost::signals2::signal<void(const blob& folderid)> folder_state_purged;
	void folder_state_purge(const blob& folderid);

	Json::Value global_state();
	Json::Value folder_state();
	Json::Value folder_state(const blob& folderid);

private:
	Json::Value global_state_buffer;
	std::map<blob, Json::Value> folder_state_buffers;
};

} /* namespace librevault */
