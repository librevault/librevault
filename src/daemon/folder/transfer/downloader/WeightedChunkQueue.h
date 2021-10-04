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
 */
#pragma once
#include <QList>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#define CLUSTERED_COEFFICIENT 10.0f
#define IMMEDIATE_COEFFICIENT 20.0f
#define RARITY_COEFFICIENT 25.0f

inline std::size_t hash_value(const QByteArray& val) { return qHash(val); }

namespace librevault {

struct FolderParams;
class MetaStorage;
class ChunkStorage;

class WeightedChunkQueue {
  struct Weight {
    bool clustered = false;
    bool immediate = false;

    int owned_by = 0;
    int remotes_count = 0;

    float value() const;
    bool operator<(const Weight& b) const { return value() > b.value(); }
    bool operator==(const Weight& b) const { return value() == b.value(); }
    bool operator!=(const Weight& b) const { return !(*this == b); }
  };
  using weight_ordered_chunks_t =
      boost::bimap<boost::bimaps::unordered_set_of<QByteArray>, boost::bimaps::multiset_of<Weight> >;
  using queue_left_value = weight_ordered_chunks_t::left_value_type;
  using queue_right_value = weight_ordered_chunks_t::right_value_type;
  weight_ordered_chunks_t weight_ordered_chunks_;

  Weight getCurrentWeight(QByteArray chunk);
  void reweightChunk(QByteArray chunk, Weight new_weight);

 public:
  void addChunk(QByteArray chunk);
  void removeChunk(QByteArray chunk);

  void setRemotesCount(int count);
  void setRemotesCount(QByteArray chunk, int count);

  void markClustered(QByteArray chunk);
  void markImmediate(QByteArray chunk);

  QList<QByteArray> chunks() const;
};

}  // namespace librevault
