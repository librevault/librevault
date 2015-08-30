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
#include "Exchanger.h"

#include "ExchangeGroup.h"
#include "fs/FSProvider.h"
#include "p2p/P2PProvider.h"

namespace librevault {

Exchanger::Exchanger(Session& session) : session_(session), log_(spdlog::get("Librevault")) {
	fs_provider_ = std::make_unique<FSProvider>(session, *this);
	p2p_provider_ = std::make_unique<P2PProvider>(session, *this);
}
Exchanger::~Exchanger() {}

void Exchanger::add(ExchangeGroup* group) {}

void Exchanger::remove(ExchangeGroup* group) {}

std::shared_ptr<ExchangeGroup> Exchanger::get(blob hash, bool create) {
	std::shared_ptr<ExchangeGroup> group_ptr;

	auto it = groups_.find(hash);
	if(it == groups_.end()) {
		if(create) group_ptr = std::make_shared<ExchangeGroup>(hash, *this);
	}else
		group_ptr = it->second->shared_from_this();
	return group_ptr;
}

} /* namespace librevault */
