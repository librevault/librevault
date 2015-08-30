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
#include "pch.h"
#pragma once

namespace librevault {

class Exchanger;
class Session {
	// Asynchronous/multithreaded operation
	io_service io_service_;

	// Logging
	std::shared_ptr<spdlog::logger> log_;

	// Components
	std::unique_ptr<Exchanger> exchanger_;

	// Program options
	fs::path config_path_;	// Config directory
	ptree config_;	// Config itself
public:
	Session(fs::path glob_config_path);
	virtual ~Session();

	void init_log();
	void init_config();

	static fs::path default_config_path();

	void run();
	void run_worker(unsigned worker_number);
	void shutdown();
	void restart();

	Exchanger& exchanger(){return *exchanger_;}

	io_service& ios(){return io_service_;}
	ptree& config(){return config_;}
	const fs::path& config_path() const {return config_path_;}
};

} /* namespace librevault */
