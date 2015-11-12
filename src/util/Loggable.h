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
#include "../pch.h"
#pragma once

namespace librevault {

class LogRoot {
public:
	logger_ptr& log() {return log_;}
protected:
	logger_ptr log_;
};

class Loggable {
protected:
	logger_ptr& log_;

	Loggable(LogRoot& log_root) : log_(log_root.log()){};
	virtual std::string log_tag() const = 0;
};

} /* namespace librevault */
