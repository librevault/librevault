#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

static const uintptr_t WINSIZE = 64;

enum class Level {
  Error = 0,
  Warn = 1,
  Info = 2,
  Debug = 3,
  Trace = 4,
};

struct FfiString {
  const uint8_t *str_p;
  uintptr_t str_len;
};

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

void fill_random(uint8_t *array, uintptr_t len);

void log_message(Level level, FfiString msg, FfiString target);

void log_init();

void rabin_append(Rabin *h, uint8_t b);

void rabin_slide(Rabin *h, uint8_t b);

void rabin_reset(Rabin *h);

bool rabin_next_chunk(Rabin *h, uint8_t b);

void rabin_init(Rabin *h);

bool rabin_finalize(Rabin *h);

} // extern "C"
