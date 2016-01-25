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
#include "NATPMPService.h"
#include "src/Client.h"
#include "src/directory/p2p/P2PProvider.h"
#include <natpmp.h>

namespace librevault {

NATPMPService::NATPMPService(Client& client, P2PProvider& provider) :
	Loggable(client, "NATPMPService"), client_(client), provider_(provider), maintain_timer_(client.ios()) {
	set_lifetime(client_.config().current.net_natpmp_lifetime);
	set_enabled(client_.config().current.net_natpmp_enabled);
}

void NATPMPService::maintain_mapping(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) return;

	if(maintain_timer_mtx_.try_lock()) {
		std::unique_lock<decltype(maintain_timer_mtx_)> maintain_timer_lk(maintain_timer_mtx_, std::adopt_lock);
		natpmp_t natpmp;
		int natpmp_ec = initnatpmp(&natpmp, 0, 0);
		log_->trace() << log_tag() << "initnatpmp() = " << natpmp_ec;

		natpmp_ec = sendnewportmappingrequest(&natpmp,
			NATPMP_PROTOCOL_TCP,
			provider_.local_endpoint().port(),
			provider_.local_endpoint().port(),
			lifetime_.count());
		log_->trace() << log_tag() << "sendnewportmappingrequest() = " << natpmp_ec;

		natpmpresp_t natpmp_resp;
		do {
			natpmp_ec = readnatpmpresponseorretry(&natpmp, &natpmp_resp);
		}while(natpmp_ec == NATPMP_TRYAGAIN);
		log_->trace() << log_tag() << "readnatpmpresponseorretry() = " << natpmp_ec;

		uint16_t public_port;
		std::chrono::seconds next_request;
		if(natpmp_ec >= 0) {
			public_port = natpmp_resp.pnu.newportmapping.mappedpublicport;
			log_->debug() << log_tag() << "Successfully set up port mapping " << provider_.local_endpoint().port() << " -> "
				<< public_port;
			next_request = std::chrono::seconds(natpmp_resp.pnu.newportmapping.lifetime);
		}else{
			public_port = 0;
			log_->debug() << log_tag() << "Could not set up port mapping";
			next_request = lifetime_;
		}

		// emit signal
		port_signal(public_port);

		maintain_timer_.expires_from_now(next_request);
		maintain_timer_.async_wait(std::bind(&NATPMPService::maintain_mapping, this, std::placeholders::_1));
	}
}

void NATPMPService::set_enabled(bool enabled) {
	if(enabled) client_.ios().post(std::bind(&NATPMPService::maintain_mapping, this, boost::system::error_code()));
}

void NATPMPService::set_lifetime(std::chrono::seconds lifetime) {
	lifetime_ = lifetime;
}

} /* namespace librevault */
