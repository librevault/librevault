#pragma once

#include <array>
#include <cstdint>
#include <boost/circular_buffer.hpp>

class Rabin {
public:
	Rabin(uint64_t polynomial, uint64_t polynomial_shift, uint64_t minsize, uint64_t maxsize, uint64_t mask, uint32_t window_size = 64);

	bool next_chunk(char b);
	bool finalize();

	uint64_t polynomial;
	uint64_t polynomial_shift;
	uint64_t minsize;
	uint64_t maxsize;
	uint64_t mask;

private:
	// Lookup tables
	std::array<uint64_t, 256> mod_table;
	std::array<uint64_t, 256> out_table;
	void calc_tables();

	// Intermediate result
	boost::circular_buffer<char> window;

	uint64_t count = 0;
	uint64_t digest = 0;

	void slide(char b);
	void append(char b);
	void reset();
};
