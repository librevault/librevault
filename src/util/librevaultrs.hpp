#pragma once

/* Generated with cbindgen:0.20.0 */

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

static const uintptr_t WINSIZE = 64;

struct Rabin {
  uint64_t mod_table[256];
  uint64_t out_table[256];
  uint8_t window[WINSIZE];
  uintptr_t wpos;
  uint64_t count;
  uint64_t pos;
  uint64_t start;
  uint64_t digest;
  uint64_t chunk_start;
  uint64_t chunk_length;
  uint64_t chunk_cut_fingerprint;
  uint64_t polynomial;
  uint64_t polynomial_degree;
  uint64_t polynomial_shift;
  uint64_t average_bits;
  uint64_t minsize;
  uint64_t maxsize;
  uint64_t mask;
};

extern "C" {

void rabin_append(Rabin *h, uint8_t b);

void rabin_slide(Rabin *h, uint8_t b);

void rabin_reset(Rabin *h);

bool rabin_next_chunk(Rabin *h, uint8_t b);

void rabin_init(Rabin *h);

bool rabin_finalize(Rabin *h);

} // extern "C"
