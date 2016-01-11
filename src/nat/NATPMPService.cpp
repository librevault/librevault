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
#include <natpmp.h>
#include "NATPMPService.h"
#include "../Client.h"
#include "../directory/Exchanger.h"
#include "../directory/p2p/P2PProvider.h"

namespace librevault {

NATPMPService::NATPMPService(Client& client, Exchanger& exchanger) :
		Loggable(client), client_(client), exchanger_(exchanger), repost_timer_(client.ios()) {
	reset_public_port();

	if(client_.config().get<bool>("net.natpmp.enabled")) perform_mapping();
}

void NATPMPService::schedule_after(std::chrono::seconds interval) {
	repost_timer_.expires_from_now(interval);
	repost_timer_.async_wait(std::bind(&NATPMPService::perform_mapping, this, std::placeholders::_1));
}

void NATPMPService::perform_mapping(const boost::system::error_code& error) {
	if(error == boost::asio::error::operation_aborted) return;

	int natpmp_ec = 0;
	natpmp_ec = initnatpmp(&natpmp_, 0, 0);
	log_->trace() << log_tag() << "initnatpmp return code:" << natpmp_ec;

	natpmp_ec = sendnewportmappingrequest(&natpmp_,
										  NATPMP_PROTOCOL_TCP,
										  exchanger_.p2p_provider()->local_endpoint().port(),
										  exchanger_.p2p_provider()->local_endpoint().port(),
										  client_.config().get<uint32_t>("net.natpmp.lifetime"));
	log_->trace() << log_tag() << "sendnewportmappingrequest return code:" << natpmp_ec;

	do {
		natpmp_ec = readnatpmpresponseorretry(&natpmp_, &natpmp_resp_);
	}while(natpmp_ec == NATPMP_TRYAGAIN);
	log_->trace() << log_tag() << "readnatpmpresponseorretry return code:" << natpmp_ec;

	if(natpmp_ec >= 0){
		public_port_ = natpmp_resp_.pnu.newportmapping.mappedpublicport;
		log_->debug() << log_tag() << "Successfully set up port mapping " << exchanger_.p2p_provider()->local_endpoint().port() << " -> " << public_port_;
		schedule_after(std::chrono::seconds(natpmp_resp_.pnu.newportmapping.lifetime));
	}else{
		reset_public_port();
		log_->debug() << log_tag() << "Could not set up port mapping";
		schedule_after(std::chrono::seconds(client_.config().get<uint32_t>("net.natpmp.lifetime")));
	}
}

void NATPMPService::reset_public_port() {
	public_port_ = exchanger_.p2p_provider()->local_endpoint().port();
}

} /* namespace librevault */
