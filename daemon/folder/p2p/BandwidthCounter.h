/* Copyright (C) 2016 Alexander Shishenko <GamePad64@gmail.com>
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
#include "pch.h"

namespace librevault {

class BandwidthCounter {
public:
	struct Stats {
		// Download
		uint64_t down_bytes_;
		uint64_t down_bytes_blocks_;

		float down_bandwidth_;
		float down_bandwidth_blocks_;

		// Upload
		uint64_t up_bytes_;
		uint64_t up_bytes_blocks_;

		float up_bandwidth_;
		float up_bandwidth_blocks_;
	};

	BandwidthCounter();

	Stats heartbeat();

	void add_down(uint64_t bytes);
	void add_down_blocks(uint64_t bytes);
	void add_up(uint64_t bytes);
	void add_up_blocks(uint64_t bytes);
private:
	std::chrono::high_resolution_clock::time_point last_heartbeat = std::chrono::high_resolution_clock::now();

	// Download
	std::atomic<uint64_t> down_bytes_;
	std::atomic<uint64_t> down_bytes_blocks_;

	std::atomic<uint64_t> down_bytes_last_;
	std::atomic<uint64_t> down_bytes_blocks_last_;

	// Upload
	std::atomic<uint64_t> up_bytes_;
	std::atomic<uint64_t> up_bytes_blocks_;

	std::atomic<uint64_t> up_bytes_last_;
	std::atomic<uint64_t> up_bytes_blocks_last_;
};

} /* namespace librevault */
