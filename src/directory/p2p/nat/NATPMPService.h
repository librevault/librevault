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
#include "../../../pch.h"
#include "../../../util/Loggable.h"

namespace librevault {

class Session;
class Exchanger;

class NATPMPService : protected Loggable {
public:
	NATPMPService(Session& session, Exchanger& exchanger);

	void perform_mapping(const boost::system::error_code& error = boost::system::error_code());

	void schedule_after(std::chrono::seconds interval);

	// Setters
	void reset_public_port();

	// Getters
	uint16_t get_public_port() const {return public_port_;}

private:
	Session& session_;
	Exchanger& exchanger_;

	boost::asio::system_timer repost_timer_;

	natpmp_t natpmp_;
	natpmpresp_t natpmp_resp_;

	uint16_t public_port_;

	std::string log_tag() const {return "[NATPMPService] ";}
};

} /* namespace librevault */
