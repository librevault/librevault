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
#pragma once
#include <miniupnpc/miniupnpc.h>
#include "PortMappingService.h"
#include "util/Loggable.h"

namespace librevault {

class Client;
class P2PProvider;

class UPnPService : public PortMappingService, public Loggable {
public:
	UPnPService(Client& client);
	~UPnPService();

	void reload_config();

	void add_port_mapping(const std::string& id, MappingDescriptor descriptor, std::string description);
	void remove_port_mapping(const std::string& id);

protected:
	Client& client_;

	// RAII wrappers
	struct DevListWrapper : boost::noncopyable {
		DevListWrapper();
		~DevListWrapper();

		UPNPDev* devlist;
	};

	class PortMapping {
	public:
		PortMapping(UPnPService& parent, MappingDescriptor descriptor, const std::string description);
		virtual ~PortMapping();

	private:
		UPnPService& parent_;
		MappingDescriptor descriptor_;

		const char* get_literal_protocol(MappingDescriptor::Protocol protocol) const {return protocol == MappingDescriptor::TCP ? "TCP" : "UDP";}
	};
	friend class PortMapping;
	std::map<std::string, std::shared_ptr<PortMapping>> mappings_;

	// Config values
	bool enabled_;

	UPNPUrls upnp_urls;
	IGDdatas upnp_data;
	std::array<char, 16> lanaddr;

	void discover_igd();
};

} /* namespace librevault */
