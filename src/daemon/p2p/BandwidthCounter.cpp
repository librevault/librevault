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
#include "BandwidthCounter.h"

namespace librevault {

BandwidthCounter::BandwidthCounter()
    : down_bytes_(0),
      down_bytes_blocks_(0),
      down_bytes_last_(0),
      down_bytes_blocks_last_(0),
      up_bytes_(0),
      up_bytes_blocks_(0),
      up_bytes_last_(0),
      up_bytes_blocks_last_(0) {
  last_heartbeat_.start();
}

BandwidthCounter::Stats BandwidthCounter::heartbeat() {
  Stats stats;

  qreal period = qreal(last_heartbeat_.restart()) / 1000;
  stats.down_bandwidth_ = qreal(down_bytes_last_.exchange(0)) / period;
  stats.down_bandwidth_blocks_ = qreal(down_bytes_blocks_last_.exchange(0)) / period;
  stats.up_bandwidth_ = qreal(up_bytes_last_.exchange(0)) / period;
  stats.up_bandwidth_blocks_ = qreal(up_bytes_blocks_last_.exchange(0)) / period;

  stats.down_bytes_ = down_bytes_;
  stats.down_bytes_blocks_ = down_bytes_blocks_;
  stats.up_bytes_ = up_bytes_;
  stats.up_bytes_blocks_ = up_bytes_blocks_;

  return stats;
}

QJsonObject BandwidthCounter::heartbeat_json() {
  BandwidthCounter::Stats traffic_stats = heartbeat();
  QJsonObject state_traffic_stats;
  state_traffic_stats["up_bandwidth"] = traffic_stats.up_bandwidth_;
  state_traffic_stats["up_bandwidth_blocks"] = traffic_stats.up_bandwidth_blocks_;
  state_traffic_stats["down_bandwidth"] = traffic_stats.down_bandwidth_;
  state_traffic_stats["down_bandwidth_blocks"] = traffic_stats.down_bandwidth_blocks_;
  state_traffic_stats["up_bytes"] = qint64(traffic_stats.up_bytes_);
  state_traffic_stats["up_bytes_blocks"] = qint64(traffic_stats.up_bytes_blocks_);
  state_traffic_stats["down_bytes"] = qint64(traffic_stats.down_bytes_);
  state_traffic_stats["down_bytes_blocks"] = qint64(traffic_stats.down_bytes_blocks_);
  return state_traffic_stats;
}

void BandwidthCounter::add_down(quint64 bytes) {
  down_bytes_ += bytes;
  down_bytes_last_ += bytes;
}

void BandwidthCounter::add_down_blocks(quint64 bytes) {
  down_bytes_blocks_ += bytes;
  down_bytes_blocks_last_ += bytes;
}

void BandwidthCounter::add_up(quint64 bytes) {
  up_bytes_ += bytes;
  up_bytes_last_ += bytes;
}

void BandwidthCounter::add_up_blocks(quint64 bytes) {
  up_bytes_blocks_ += bytes;
  up_bytes_blocks_last_ += bytes;
}

}  // namespace librevault
