/*
 * You may redistribute this program and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This is an implementation of Kademlia DHT, found here:
 * http://xlattice.sourceforge.net/components/protocol/kademlia/specs.html
 */

#include "DHTService.h"
#include <algorithm>
#include <cmath>

namespace p2pnet {
namespace dht {

/* Constructors/Destructors */
DHTService::DHTService(uint16_t alpha,
		uint16_t k,
		uint16_t B,
		std::chrono::seconds tExpire,
		std::chrono::seconds tRefresh,
		std::chrono::seconds tReplicate,
		std::chrono::seconds tRepublish) :
		alpha(alpha), k(k), B(B), tExpire(tExpire), tRefresh(tRefresh), tReplicate(tReplicate), tRepublish(tRepublish) {
}

DHTService::~DHTService() {}

std::list<DHTNode*> DHTService::getClosestTo(const crypto::Hash& hash, int16_t count) {
	return getClosestTo(hash, count);
}

void DHTService::findAny(const crypto::Hash& find_hash, const crypto::Hash& dest_hash, protocol::DHTPart::DHTMessageType action) {
	protocol::DHTPart dht_part;
	dht_part.set_message_type(action);
	dht_part.set_hash(find_hash.toBinaryString());

	send(dest_hash, dht_part);
}

void DHTService::findAnyIterative(const crypto::Hash& find_hash, protocol::DHTPart::DHTMessageType action) {
	auto hash_shared_ptr = std::make_shared<const crypto::Hash>(find_hash);
	auto& this_search = searches[hash_shared_ptr];
	this_search.searching_hash = hash_shared_ptr;

	for(auto node : k_buckets->getClosest(find_hash, alpha)){
		this_search.shortlist[node] = Search::READY;

		if(this_search.closest_node){
			auto closest_distance = this_search.closest_node->getHash() ^ find_hash;
			if(node->getHash() ^ find_hash < closest_distance){
				this_search.closest_node = node;
			}
		}else{
			this_search.closest_node = node;
		}
	}

	for(auto node : this_search.shortlist){
		if(node.second == Search::READY){
			findAny(*(this_search.searching_hash), node.first->getHash(), action);
		}
	}
}

/* Send/Process */
void DHTService::process(const crypto::Hash& from, const protocol::DHTPart& dht_part){
	if(dht_part.message_type() == dht_part.FIND_NODE || dht_part.message_type() == dht_part.FIND_VALUE){
		crypto::Hash query_hash;
		query_hash.setAsBinaryString(dht_part.hash());

		protocol::DHTPart reply;
		reply.set_ns(system_ns);
		reply.set_hash(dht_part.hash());

		bool found_value = false;
		if(dht_part.message_type() == dht_part.FIND_NODE){
			reply.set_message_type(dht_part.FIND_NODE_REPLY);
		}else if(dht_part.message_type() == dht_part.FIND_VALUE){
			reply.set_message_type(dht_part.FIND_VALUE_REPLY);

			//TODO
		}

		if(!found_value){
			auto kClosest = getClosestTo(query_hash, k);

			for(auto node : kClosest){
				reply.add_serialized_contact_list(node->getSerializedContact());
			}
		}

		send(from, reply);
	} else if(dht_part.message_type() == dht_part.FIND_NODE_REPLY){
		for(auto contact : dht_part.serialized_contact_list()){
			foundNode(contact);
		}
	} else if(dht_part.message_type() == dht_part.FIND_VALUE_REPLY){
		//TODO
	}
}

void DHTService::findNode(const crypto::Hash& find_hash, const crypto::Hash& dest_hash) {
	findAny(find_hash, dest_hash, protocol::DHTPart::FIND_NODE);
}

void DHTService::findValue(const crypto::Hash& find_hash, const crypto::Hash& dest_hash) {
	findAny(find_hash, dest_hash, protocol::DHTPart::FIND_VALUE);
}

void DHTService::storeValue(const std::string& value, const crypto::Hash& dest_hash) {
}

void DHTService::pingNode(const crypto::Hash& dest_hash) {
}

void DHTService::findNodeIterative(const crypto::Hash& find_hash) {
	findAnyIterative(find_hash, protocol::DHTPart::FIND_NODE);
}

void DHTService::findValueIterative(const crypto::Hash& find_hash) {
	findAnyIterative(find_hash, protocol::DHTPart::FIND_VALUE);
}

void DHTService::storeValueIterative(const std::string& value) {
}

} /* namespace dht */
} /* namespace p2pnet */
