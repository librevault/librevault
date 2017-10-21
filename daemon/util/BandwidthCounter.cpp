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
#include <QtCore/QPoint>

namespace librevault {

BandwidthCounter::BandwidthCounter(BandwidthCounter* parent_counter)
    : parent_counter_(parent_counter) {
  last_heartbeat_.start();
}

QJsonObject BandwidthCounter::heartbeat_json() {
  QMutexLocker lk(&mutex_);

  qreal period = qreal(last_heartbeat_.restart()) / 1000;

  QJsonObject state_traffic_stats;
  state_traffic_stats["up_bandwidth"] = qreal(upload_bytes_last_) / period;
  state_traffic_stats["down_bandwidth"] = qreal(download_bytes_last_) / period;
  state_traffic_stats["up_bytes"] = (qreal)upload_bytes_;
  state_traffic_stats["down_bytes"] = (qreal)download_bytes_;

  upload_bytes_last_ = 0;
  download_bytes_last_ = 0;

  return state_traffic_stats;
}

void BandwidthCounter::add_down(quint64 bytes) {
  QMutexLocker lk(&mutex_);

  download_bytes_ += bytes;
  download_bytes_last_ += bytes;
  if (parent_counter_) parent_counter_->add_down(bytes);
}

void BandwidthCounter::add_up(quint64 bytes) {
  QMutexLocker lk(&mutex_);

  upload_bytes_ += bytes;
  download_bytes_ += bytes;
  if (parent_counter_) parent_counter_->add_up(bytes);
}

} /* namespace librevault */
