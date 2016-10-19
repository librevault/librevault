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
#include <util/log_scope.h>
#include <util/network.h>
#include <util/multi_io_service.h>

#include <boost/signals2.hpp>
#include <mutex>

namespace librevault {

class NATPMPService;
class UPnPService;
class PortMappingSubService;

class PortMappingService {
	LOG_SCOPE("PortMappingService");
	friend class PortMappingSubService;
public:
	struct MappingDescriptor {
		uint16_t port;
		int protocol;
	};

	PortMappingService();
	virtual ~PortMappingService();

	void run() {io_service_.start(1);}
	void stop() {io_service_.stop();}

	void add_port_mapping(std::string id, MappingDescriptor descriptor, std::string description);
	void remove_port_mapping(std::string id);
	uint16_t get_port_mapping(const std::string& id);

private:
	multi_io_service io_service_;

	std::mutex mappings_mutex_;
	struct Mapping {
		MappingDescriptor descriptor;
		std::string description;
		uint16_t port;
	};
	std::map<std::string, Mapping> mappings_;

	std::unique_ptr<NATPMPService> natpmp_service_;
	std::unique_ptr<UPnPService> upnp_service_;

	void add_existing_mappings(PortMappingSubService* subservice);
};

} /* namespace librevault */
