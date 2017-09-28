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
#include "WeightedChunkQueue.h"
#include <QLoggingCategory>
#include <boost/range/adaptor/map.hpp>

namespace librevault {

float WeightedChunkQueue::Weight::value() const {
	float weight_value = 0;

	weight_value += CLUSTERED_COEFFICIENT * (clustered ? 1 : 0);
	weight_value += IMMEDIATE_COEFFICIENT * (immediate ? 1 : 0);
	float rarity = (float)(peer_count - owned_by) / (float)peer_count;
	weight_value += rarity * RARITY_COEFFICIENT;

	return weight_value;
}

WeightedChunkQueue::Weight WeightedChunkQueue::getCurrentWeight(QByteArray chunk) {
	auto it = weight_ordered_chunks_.left.find(chunk);
	if(it != weight_ordered_chunks_.left.end())
		return it->second;
	else
		return Weight();
}

void WeightedChunkQueue::reweightChunk(QByteArray chunk, Weight new_weight) {
	auto chunk_it = weight_ordered_chunks_.left.find(chunk);

	if(chunk_it != weight_ordered_chunks_.left.end() && chunk_it->second != new_weight) {
		weight_ordered_chunks_.left.erase(chunk_it);
		weight_ordered_chunks_.left.insert(queue_left_value(chunk, new_weight));
	}
}

void WeightedChunkQueue::addChunk(QByteArray chunk) {
	weight_ordered_chunks_.left.insert(queue_left_value(chunk, Weight()));
}

void WeightedChunkQueue::removeChunk(QByteArray chunk) {
	weight_ordered_chunks_.left.erase(chunk);
}

void WeightedChunkQueue::setPeerCount(int count) {
	weight_ordered_chunks_t new_queue;
	for(auto& entry : weight_ordered_chunks_.left) {
		QByteArray chunk = entry.first;
		Weight weight = entry.second;

		weight.peer_count = count;
		new_queue.left.insert(queue_left_value(chunk, weight));
	}
	weight_ordered_chunks_ = new_queue;
}

void WeightedChunkQueue::setPeerCount(QByteArray chunk, int count) {
	Weight weight = getCurrentWeight(chunk);
	weight.owned_by = count;

	reweightChunk(chunk, weight);
}

void WeightedChunkQueue::markClustered(QByteArray chunk) {
	Weight weight = getCurrentWeight(chunk);
	weight.clustered = true;

	reweightChunk(chunk, weight);
}

void WeightedChunkQueue::markImmediate(QByteArray chunk) {
	Weight weight = getCurrentWeight(chunk);
	weight.immediate = true;

	reweightChunk(chunk, weight);
}

QList<QByteArray> WeightedChunkQueue::chunks() const {
	QList<QByteArray> chunk_list;
	chunk_list.reserve(weight_ordered_chunks_.size());
	for(auto& chunk_ptr : boost::adaptors::values(weight_ordered_chunks_.right))
		chunk_list << chunk_ptr;
	return chunk_list;
}

} /* namespace librevault */
