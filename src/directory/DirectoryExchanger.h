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

class AbstractDirectory;
class AbstractProvider;
class FSDirectory;
class FSProvider;
class P2PDirectory;
class P2PProvider;

class Session;

class DirectoryExchanger {
public:
	DirectoryExchanger(Session& session);
	virtual ~DirectoryExchanger();

	void register_directory(std::shared_ptr<AbstractDirectory> directory);
	void new_revision(std::shared_ptr<AbstractDirectory> directory, int64_t revision);

private:
	Session& session_;
	std::shared_ptr<spdlog::logger> log_;

	std::unique_ptr<FSProvider> fs_provider_;
	std::unique_ptr<P2PProvider> p2p_provider_;

	std::set<blob> allowed_hashes_;
};

} /* namespace librevault */
