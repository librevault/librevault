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
#ifndef DHTSERVICE_H_
#define DHTSERVICE_H_

#define MAX_NODES_TOTAL 30
#define MAX_FROM_BUCKET 5

#include "KBucket.h"
#include "DHTNode.h"

#include "../../common/crypto/Hash.h"
#include "DHT.pb.h"

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <unordered_map>
#include <list>
#include <set>

namespace p2pnet {
namespace dht {

/**
 * This is basic implementation of Kademlia DHT, specifications defined here:
 * http://xlattice.sourceforge.net/components/protocol/kademlia/specs.html
 *
 * Constants, such as: alpha, k, B, tExpire, tRefresh, tReplicate, tRepublish
 * are defined just like in the original paper, except for B, as we use
 * SHA3-224 (224 bit long) instead of SHA-1 (160 bit long).
 */
class DHTService : boost::noncopyable {
protected:
	/* Kademlia definition constants */
	const uint16_t alpha, k, B;
	const std::chrono::seconds tExpire, tRefresh, tReplicate, tRepublish;

	struct Search {
		enum RPC_Status {
			READY = 0,
			SENT_REQUEST
		};

		std::shared_ptr<const crypto::Hash> searching_hash;
		DHTNode* closest_node;
		std::map<DHTNode*, RPC_Status> shortlist;

		protocol::DHTPart::DHTMessageType dht_action_type;
	};
	std::map<std::shared_ptr<const crypto::Hash>, Search> searches;

	/* K-buckets themselves */
	std::unique_ptr<KBucket> k_buckets;

	// Other constants
	const std::string system_ns = "system";

	/**
	 * Protected constructor, as we don't suppose to create DHTService objects.
	 * All child classes MUST create an instance of KBucket by themselves.
	 * Its parameters are Kademlia definition constants.
	 * @param alpha measure of parallelism, number of asynchronous network calls
	 * @param k number of nodes in bucket
	 * @param B bits in NodeID (aka crypto::Hash)
	 * @param tExpire the time after which a key/value pair expires; this is a time-to-live (TTL) from the original publication date
	 * @param tRefresh after which an otherwise unaccessed bucket must be refreshed
	 * @param tReplicate the interval between Kademlia replication events, when a node is required to publish its entire database
	 * @param tRepublish the time after which the original publisher must republish a key/value pair
	 */
	DHTService(
			uint16_t alpha = 3,
			uint16_t k = 20,
			uint16_t B = 224,
			std::chrono::seconds tExpire = std::chrono::seconds(86400),
			std::chrono::seconds tRefresh = std::chrono::seconds(3600),
			std::chrono::seconds tReplicate = std::chrono::seconds(3600),
			std::chrono::seconds tRepublish = std::chrono::seconds(86410)	// See note in http://xlattice.sourceforge.net/components/protocol/kademlia/specs.html#constants
			);

	virtual crypto::Hash getMyHash() = 0;
	virtual std::list<DHTNode*> getClosestTo(const crypto::Hash& hash, int16_t count);

	virtual void findAny(const crypto::Hash& find_hash, const crypto::Hash& dest_hash, protocol::DHTPart::DHTMessageType action);
	virtual void findAnyIterative(const crypto::Hash& find_hash, protocol::DHTPart::DHTMessageType action);
public:
	virtual ~DHTService();

	/* Send/process */
	virtual void send(const crypto::Hash& dest, const protocol::DHTPart& dht_part) = 0;
	void process(const crypto::Hash& from, const protocol::DHTPart& dht_part);

	/* Primitive actions */
	virtual void findNode(const crypto::Hash& find_hash, const crypto::Hash& dest_hash);
	virtual void findValue(const crypto::Hash& find_hash, const crypto::Hash& dest_hash);
	virtual void storeValue(const std::string& value, const crypto::Hash& dest_hash);
	virtual void pingNode(const crypto::Hash& dest_hash);

	/* Iterative actions */
	virtual void findNodeIterative(const crypto::Hash& find_hash);
	virtual void findValueIterative(const crypto::Hash& find_hash);
	virtual void storeValueIterative(const std::string& value);

	/* Callbacks */
	virtual void foundNode(std::string serialized_contact) = 0;
};

} /* namespace dht */
} /* namespace p2pnet */

#endif /* DHTSERVICE_H_ */
