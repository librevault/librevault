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
#include "src/folder/p2p/P2PProvider.h"
#include "src/folder/p2p/WSServer.h"
#include <natpmp.h>

namespace librevault {

NATPMPService::NATPMPService(Client& client, P2PProvider& provider) :
	Loggable(client, "NATPMPService"), client_(client), provider_(provider), maintain_timer_(client.network_ios()) {

	reload_config();
	Config::get()->config_changed.connect(std::bind(&NATPMPService::reload_config, this));
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
			provider_.ws_server()->local_endpoint().port(),
			provider_.ws_server()->local_endpoint().port(), // TODO: invoke on port change (maybe, signal?)
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
			log_->debug() << log_tag() << "Successfully set up port mapping " << provider_.ws_server()->local_endpoint().port() << " -> "
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

void NATPMPService::reload_config() {
	bool new_enabled = Config::get()->client()["natpmp_enabled"].asBool();
	seconds new_lifetime = seconds(Config::get()->client()["natpmp_lifetime"].asUInt64());

	if((new_lifetime != lifetime_ || new_enabled != enabled_) && new_enabled)
		client_.ios().post(std::bind(&NATPMPService::maintain_mapping, this, boost::system::error_code()));

	enabled_ = new_enabled;
	lifetime_ = new_lifetime;
}

} /* namespace librevault */
