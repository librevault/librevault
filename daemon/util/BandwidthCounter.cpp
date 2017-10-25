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
#include "BandwidthCounter.h"

namespace librevault {

QJsonObject BandwidthStats::toJson() const {
  QJsonObject stats_json;
  stats_json["up_bandwidth"] = up_bandwidth;
  stats_json["down_bandwidth"] = down_bandwidth;
  stats_json["up_bytes"] = up_bytes;
  stats_json["down_bytes"] = down_bytes;
  return stats_json;
}

BandwidthStats BandwidthStats::fromJson(const QJsonObject& stats_json) {
  BandwidthStats stats{};
  stats.up_bandwidth = stats_json["up_bandwidth"].toDouble();
  stats.down_bandwidth = stats_json["down_bandwidth"].toDouble();
  stats.up_bytes = stats_json["up_bytes"].toDouble();
  stats.down_bytes = stats_json["down_bytes"].toDouble();
  return stats;
}

BandwidthCounter::BandwidthCounter(BandwidthCounter* parent_counter)
    : parent_counter_(parent_counter) {
  last_heartbeat_.start();
}

BandwidthStats BandwidthCounter::makeStats() {
  QMutexLocker lk(&mutex_);

  double period = double(last_heartbeat_.restart()) / 1000;

  BandwidthStats stats{};
  stats.up_bandwidth = double(up_bytes_last_) / period;
  stats.down_bandwidth = double(down_bytes_last_) / period;
  stats.up_bytes = up_bytes_;
  stats.down_bytes = down_bytes_;

  up_bytes_last_ = 0;
  down_bytes_last_ = 0;

  return stats;
}

void BandwidthCounter::addDown(quint64 bytes) {
  QMutexLocker lk(&mutex_);

  down_bytes_ += bytes;
  down_bytes_last_ += bytes;
  if (parent_counter_) parent_counter_->addDown(bytes);
}

void BandwidthCounter::addUp(quint64 bytes) {
  QMutexLocker lk(&mutex_);

  up_bytes_ += bytes;
  up_bytes_last_ += bytes;
  if (parent_counter_) parent_counter_->addUp(bytes);
}

} /* namespace librevault */
