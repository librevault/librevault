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

#include "../util/Loggable.h"

namespace librevault {

class Session;

class FSDirectory;
class P2PProvider;
class MulticastDiscovery;

class ExchangeGroup;

class Exchanger : protected Loggable {
public:
	Exchanger(Session& session);
	virtual ~Exchanger();

	void register_group(std::shared_ptr<ExchangeGroup> group_ptr);
	void unregister_group(std::shared_ptr<ExchangeGroup> group_ptr);

	std::shared_ptr<ExchangeGroup> get_group(const blob& hash);

	P2PProvider* get_p2p_provider();
private:
	Session& session_;

	// Remote
	std::unique_ptr<P2PProvider> p2p_provider_;

	std::map<blob, std::shared_ptr<ExchangeGroup>> hash_group_;

	std::unique_ptr<MulticastDiscovery> multicast4_, multicast6_;

	std::string log_tag() const {return "Exchanger";}

	void add_directory(const ptree& dir_options);
};

} /* namespace librevault */
