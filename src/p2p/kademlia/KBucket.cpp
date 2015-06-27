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
#include "KBucket.h"

namespace librevault {
namespace p2p {

KBucket::KBucket(const blob& node_hash, uint16_t k) : index(0), splittable(true), k(k) {
	node_hash_ptr = std::make_shared<const blob>(node_hash);
}

KBucket::KBucket(int index, bool splittable, std::shared_ptr<const blob> node_hash_ptr, uint16_t k) :
		index(index),
		splittable(splittable),
        node_hash_ptr(node_hash_ptr), k(k) {}

KBucket::~KBucket() {}

void KBucket::split() {
	if(is_split() || !splittable)
		return;

	auto my_hash_bit = get_bit(*node_hash_ptr, index+1);

	split_buckets.first = std::unique_ptr<KBucket>(new KBucket(index+1, !my_hash_bit, node_hash_ptr, k));
	split_buckets.second = std::unique_ptr<KBucket>(new KBucket(index+1, my_hash_bit, node_hash_ptr, k));

	for(auto node : bucket_contents){
		determine_bucket(node->get_id())->bucket_contents.push_back(node);
	}

	bucket_contents.clear();
}

KBucket* KBucket::determine_bucket(const blob& hash) {
	if(!is_split())
		return this;

	if( get_bit(hash, index+1) == false )
		return split_buckets.first.get();
	else
		return split_buckets.second.get();
}

void KBucket::cleanup(){
	for(auto it = bucket_contents.begin(); it != bucket_contents.end(); it++)
		if((*it)->get_quality() == Node::BAD)
			bucket_contents.erase(it);
}

bool KBucket::get_bit(const blob& hash, int bit_index){
	int split_byte = bit_index / 8;
	int split_bit = bit_index % 8;

	return (hash[split_byte] >> (7-split_bit)) & 0x1;
}

bool KBucket::is_split() const {
	return split_buckets.first != nullptr && split_buckets.second != nullptr;
}

void KBucket::add_node(std::shared_ptr<Node> node, bool force) {
	auto node_hash = node->get_id();

	if(!is_split()){
		if(bucket_contents.size() < k){	// Then we don't need this cleanup routine
			bucket_contents.push_back(node);
		}else{
			// Okay, we need to cleanup. So, we leave only GOOD nodes.
			std::remove_if(bucket_contents.begin(), bucket_contents.end(),
					[](decltype(bucket_contents)::value_type pred_node){return pred_node->get_quality() == Node::BAD;}
			);

			for(auto node_ptr : bucket_contents){
				if(node_ptr == node)
					return;	// This node is already here
			}

			if(bucket_contents.size() >= k && splittable){
				split();
				determine_bucket(node_hash)->add_node(node, force);
			}else if(bucket_contents.size() < k || force){
				bucket_contents.push_back(node);
			}
		}
	}else{
		determine_bucket(node_hash)->add_node(node, force);
	}
}

void KBucket::remove_node(std::shared_ptr<Node> node) {
	auto node_hash = node->get_id();

	if(!is_split()){
		auto erase_it = std::find(bucket_contents.begin(), bucket_contents.end(), node);
		if(erase_it != bucket_contents.end())
			bucket_contents.erase(erase_it);
	}else{
		determine_bucket(node_hash)->remove_node(node);
	}
}

std::list<std::shared_ptr<Node>> KBucket::get_closest(const blob& hash, int n) {
	std::list<std::shared_ptr<Node>> result_list;

	if(!is_split()){
		decltype(n) i = 0;
		auto it = bucket_contents.begin();
		while(i < n || it != bucket_contents.end()){
			result_list.push_back(*it);
			i++; it++;
		}
		return result_list;
	}else{
		auto first = determine_bucket(hash);
        auto& second = (split_buckets.first.get() == first ? split_buckets.second : split_buckets.first);

		result_list = first->get_closest(hash, n);

		if(result_list.size() < n){
			std::list<std::shared_ptr<Node>> result_list_appendix = second->get_closest(hash, n);
			result_list.insert(result_list.end(), result_list_appendix.begin(), result_list_appendix.end());
		}

		return result_list;
	}
}

void KBucket::rebuild(const blob& node_hash, const std::set<std::shared_ptr<Node>>& node_list) {
	bucket_contents.clear();
	split_buckets = {nullptr, nullptr};

	for(auto node_ptr : node_list){
		add_node(node_ptr);
	}
}

int KBucket::count() const {
	if(is_split()){
		return split_buckets.first->count() + split_buckets.second->count();
	}else{
		return bucket_contents.size();
	}
}

} /* namespace overlay */
} /* namespace librevault */
