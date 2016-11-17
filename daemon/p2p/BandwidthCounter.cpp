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
#include <json/json.h>

namespace librevault {

BandwidthCounter::BandwidthCounter() : down_bytes_(0), down_bytes_blocks_(0), down_bytes_last_(0), down_bytes_blocks_last_(0),
    up_bytes_(0), up_bytes_blocks_(0), up_bytes_last_(0), up_bytes_blocks_last_(0) {}

BandwidthCounter::Stats BandwidthCounter::heartbeat() {
	Stats stats;

	std::chrono::duration<float> since_last = std::chrono::high_resolution_clock::now() - last_heartbeat;
	stats.down_bandwidth_ = float(down_bytes_last_.exchange(0)) / since_last.count();
	stats.down_bandwidth_blocks_ = float(down_bytes_blocks_last_.exchange(0)) / since_last.count();
	stats.up_bandwidth_ = float(up_bytes_last_.exchange(0)) / since_last.count();
	stats.up_bandwidth_blocks_ = float(up_bytes_blocks_last_.exchange(0)) / since_last.count();

	stats.down_bytes_ = down_bytes_;
	stats.down_bytes_blocks_ = down_bytes_blocks_;
	stats.up_bytes_ = up_bytes_;
	stats.up_bytes_blocks_ = up_bytes_blocks_;

	last_heartbeat = std::chrono::high_resolution_clock::now();

	return stats;
}

Json::Value BandwidthCounter::heartbeat_json() {
	BandwidthCounter::Stats traffic_stats = heartbeat();
	Json::Value state_traffic_stats;
	state_traffic_stats["up_bandwidth"] = traffic_stats.up_bandwidth_;
	state_traffic_stats["up_bandwidth_blocks"] = traffic_stats.up_bandwidth_blocks_;
	state_traffic_stats["down_bandwidth"] = traffic_stats.down_bandwidth_;
	state_traffic_stats["down_bandwidth_blocks"] = traffic_stats.down_bandwidth_blocks_;
	state_traffic_stats["up_bytes"] = Json::Value::UInt64(traffic_stats.up_bytes_);
	state_traffic_stats["up_bytes_blocks"] = Json::Value::UInt64(traffic_stats.up_bytes_blocks_);
	state_traffic_stats["down_bytes"] = Json::Value::UInt64(traffic_stats.down_bytes_);
	state_traffic_stats["down_bytes_blocks"] = Json::Value::UInt64(traffic_stats.down_bytes_blocks_);
	return state_traffic_stats;
}

void BandwidthCounter::add_down(uint64_t bytes) {
	down_bytes_ += bytes;
	down_bytes_last_ += bytes;
}

void BandwidthCounter::add_down_blocks(uint64_t bytes) {
	down_bytes_blocks_ += bytes;
	down_bytes_blocks_last_ += bytes;
}

void BandwidthCounter::add_up(uint64_t bytes) {
	up_bytes_ += bytes;
	up_bytes_last_ += bytes;
}

void BandwidthCounter::add_up_blocks(uint64_t bytes) {
	up_bytes_blocks_ += bytes;
	up_bytes_blocks_last_ += bytes;
}

} /* namespace librevault */
