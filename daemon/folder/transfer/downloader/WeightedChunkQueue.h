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
#include <QList>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#define CLUSTERED_COEFFICIENT 10.0f
#define IMMEDIATE_COEFFICIENT 20.0f
#define RARITY_COEFFICIENT 25.0f

inline std::size_t hash_value(const QByteArray& val) {
	return qHash(val);
}

namespace librevault {

class FolderSettings;
class Storage;
class ChunkStorage;

class WeightedChunkQueue {
	struct Weight {
		bool clustered = false;
		bool immediate = false;

		int owned_by = 0;
		int peer_count = 0;

		float value() const;
		bool operator<(const Weight& b) const {return value() > b.value();}
		bool operator==(const Weight& b) const {return value() == b.value();}
		bool operator!=(const Weight& b) const {return !(*this == b);}
	};
	using weight_ordered_chunks_t = boost::bimap<
		boost::bimaps::unordered_set_of<QByteArray>,
		boost::bimaps::multiset_of<Weight>
	>;
	using queue_left_value = weight_ordered_chunks_t::left_value_type;
	using queue_right_value = weight_ordered_chunks_t::right_value_type;
	weight_ordered_chunks_t weight_ordered_chunks_;

	Weight getCurrentWeight(QByteArray chunk);
	void reweightChunk(QByteArray chunk, Weight new_weight);

public:
	void addChunk(QByteArray chunk);
	void removeChunk(QByteArray chunk);

	void setPeerCount(int count);
	void setPeerCount(QByteArray chunk, int count);

	void markClustered(QByteArray chunk);
	void markImmediate(QByteArray chunk);

	QList<QByteArray> chunks() const;
};

} /* namespace librevault */
