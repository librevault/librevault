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
#include "../pch.h"
#pragma once
#include "Loggable.h"

namespace librevault {

class multi_io_service : protected Loggable {
public:
	multi_io_service(Loggable& parent_loggable, const std::string& name);
	~multi_io_service();

	void start(unsigned thread_count);
	void stop();

	io_service& ios() {return ios_;}

protected:
	io_service ios_;
	std::unique_ptr<io_service::work> ios_work_;

	std::vector<std::thread> worker_threads_;

	void run_thread(unsigned worker_number);

	std::string log_tag() const {return std::string("[pool:") + name_ + "] ";}
};

} /* namespace librevault */
