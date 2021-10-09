#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

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

extern "C" {

void fill_random(uint8_t *array, uintptr_t len);

void log_message(Level level, FfiString msg, FfiString target);

void log_init();

} // extern "C"
