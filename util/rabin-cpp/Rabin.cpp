#include "Rabin.hpp"

namespace {

int deg(uint64_t p) {
  uint64_t mask = 0x8000000000000000LL;

  for(int i = 0; i < 64; i++) {
    if((mask & p) > 0) {
      return 63 - i;
    }

    mask >>= 1;
  }

  return -1;
}

// Mod calculates the remainder of x divided by p.
uint64_t mod(uint64_t x, uint64_t p) {
  while(deg(x) >= deg(p)) {
    uint64_t shift = deg(x) - deg(p);

    x = x ^ (p << shift);
  }

  return x;
}

uint64_t append_byte(uint64_t hash, uint8_t b, uint64_t pol) {
  hash <<= 8;
  hash |= (uint64_t)b;

  return mod(hash, pol);
}

}

Rabin::Rabin(uint64_t polynomial, uint64_t polynomial_shift, uint64_t minsize, uint64_t maxsize, uint64_t mask, uint32_t window_size) :
  polynomial(polynomial), polynomial_shift(polynomial_shift), minsize(minsize), maxsize(maxsize), mask(mask), window(window_size) {
  calc_tables();
  reset();
}

void Rabin::slide(char b) {
  uint8_t out = window.empty() ? 0 : window.front();
  window.push_back(b);

  digest = (digest ^ out_table[out]);
  append(b);
}

void Rabin::append(char b) {
  uint8_t index = (uint8_t)(digest >> polynomial_shift);
  digest <<= 8;
  digest |= (uint64_t)b;
  digest ^= mod_table[index];
}

bool Rabin::next_chunk(char b) {
  slide(b);

  count++;

  if((count >= minsize && ((digest & mask) == 0)) || count >= maxsize) {
    Rabin::reset();
    return true;
  }

  return false;
}

bool Rabin::finalize() {
  auto now_count = count;
  reset();
  return now_count > 0;
}

void Rabin::reset() {
  window.clear();

  count = 0;
  digest = 0;

  slide(1);
}

void Rabin::calc_tables() {
  // calculate table for sliding out bytes. The byte to slide out is used as
  // the index for the table, the value contains the following:
  // out_table[b] = Hash(b || 0 ||        ...        || 0)
  //                          \ windowsize-1 zero bytes /
  // To slide out byte b_0 for window size w with known hash
  // H := H(b_0 || ... || b_w), it is sufficient to add out_table[b_0]:
  //    H(b_0 || ... || b_w) + H(b_0 || 0 || ... || 0)
  //  = H(b_0 + b_0 || b_1 + 0 || ... || b_w + 0)
  //  = H(    0     || b_1 || ...     || b_w)
  //
  // Afterwards a new byte can be shifted in.
  for(int b = 0; b < 256; b++) {
    uint64_t hash = 0;

    hash = append_byte(hash, (uint8_t)b, polynomial);
    for(size_t i = 0; i < window.capacity() - 1; i++) {
      hash = append_byte(hash, 0, polynomial);
    }
    out_table[b] = hash;
  }

  // calculate table for reduction mod Polynomial
  int k = deg(polynomial);
  for(int b = 0; b < 256; b++) {
    // mod_table[b] = A | B, where A = (b(x) * x^k mod pol) and  B = b(x) * x^k
    //
    // The 8 bits above deg(Polynomial) determine what happens next and so
    // these bits are used as a lookup to this table. The value is split in
    // two parts: Part A contains the result of the modulus operation, part
    // B is used to cancel out the 8 top bits so that one XOR operation is
    // enough to reduce modulo Polynomial
    mod_table[b] = mod(((uint64_t)b) << k, polynomial) | ((uint64_t)b) << k;
  }
}
