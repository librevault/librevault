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
#pragma once

#include <list>
#include <set>
#include <memory>
#include <vector>

#include "../Node.h"

namespace librevault {
namespace p2p {

using blob = std::vector<uint8_t>;

/**
 * It is basically an implementation of BitTorrent-like Kademlia K-buckets.
 *
 * See more about K-buckets:
 * JS implementation of K-buckets: https://github.com/tristanls/k-bucket
 * BitTorrent BEP-5: http://www.bittorrent.org/beps/bep_0005.html
 */
class KBucket {
	const int index;
	const uint16_t k;
	std::pair<std::unique_ptr<KBucket>, std::unique_ptr<KBucket>> split_buckets = {nullptr, nullptr};
	std::list<std::shared_ptr<Node>> bucket_contents;
	const bool splittable;

	std::shared_ptr<const blob> node_hash_ptr;

	KBucket(int index, bool splittable, std::shared_ptr<const blob> node_hash_ptr, uint16_t k);

	/**
	 * Splits this KBucket into two. Uses index to determine, by which byte shall we split.
	 */
	void split();

	/**
	 * Returns pointer to KBucket, which can store this hash. If KBucket is not split yet, it returns "this"
	 * @param hash
	 * @return
	 */
	KBucket* determine_bucket(const blob& hash);

	void cleanup();

	/**
	 * Gets specific bit in hash
	 * @param hash
	 * @param bit_index
	 * @return
	 *
	 * TODO: Watch behavior of this function on machines with different endianess.
	 */
	bool get_bit(const std::vector<uint8_t>& hash, int bit_index);

	bool is_split() const;
public:
	/**
	 * KBucket public constructor. It constructs new KBucket, using node_hash as its own node's hash.
	 * @param node_hash Own node's hash.
	 * @param k K parameter, number of nodes in each KBucket binary tree leaf.
	 */
	KBucket(const blob& node_hash, uint16_t k);
	virtual ~KBucket();	// Unique_ptr's will delete nested k-buckets recursively!

	void add_node(std::shared_ptr<Node> node, bool force = false);
	void remove_node(std::shared_ptr<Node> node);
	std::list<std::shared_ptr<Node>> get_closest(const blob& hash, int n = 0);

	void rebuild(const blob& node_hash, const std::set<std::shared_ptr<Node>>& node_list);

	/**
	 * Counts elements in this KBucket. This function walks around all children KBuckets.
	 * @return Elements in KBucket.
	 */
	int count() const;
};

} /* namespace overlay */
} /* namespace librevault */
