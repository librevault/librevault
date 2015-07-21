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
#include "DirectoryExchanger.h"
#include "fs/FSProvider.h"
#include "p2p/P2PProvider.h"

namespace librevault {

DirectoryExchanger::DirectoryExchanger(Session& session) : session_(session), log_(spdlog::get("Librevault")) {
	fs_provider_ = std::make_unique<FSProvider>(session, *this);
	p2p_provider_ = std::make_unique<P2PProvider>(session, *this);
}
DirectoryExchanger::~DirectoryExchanger() {}

void DirectoryExchanger::register_directory(std::shared_ptr<AbstractDirectory> directory){
	log_->debug() << "[DirectoryExchanger] " << "Directory registered";
}

} /* namespace librevault */
