#include "random.h"

#include <cryptopp/osrng.h>

QByteArray randomBytes(int size) {
  QByteArray data(size, '\0');
  CryptoPP::AutoSeededRandomPool().GenerateBlock(reinterpret_cast<CryptoPP::byte*>(data.data()), data.size());
  return data;
}
