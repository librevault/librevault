#include "random.h"

#include <cryptopp/osrng.h>

QByteArray randomBytes(int size) {
  thread_local auto pool = CryptoPP::AutoSeededRandomPool();

  QByteArray data(size, '\0');
  pool.GenerateBlock(reinterpret_cast<CryptoPP::byte*>(data.data()), data.size());
  return data;
}
