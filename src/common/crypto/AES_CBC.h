#pragma once
#include <cryptopp/osrng.h>
#include <util/random.h>

#include "Transformer.h"

namespace librevault::crypto {

class AES_CBC : public TwoWayTransformer {
 private:
  const QByteArray key;
  const QByteArray iv;
  bool padding;

 public:
  AES_CBC(const QByteArray& key, const QByteArray& iv, bool padding = true);
  ~AES_CBC() override = default;

  QByteArray encrypt(const QByteArray& plaintext) const;
  QByteArray decrypt(const QByteArray& ciphertext) const;

  static QByteArray randomIv() { return randomBytes(16); }

  QByteArray to(const QByteArray& data) const { return encrypt(data); }
  QByteArray from(const QByteArray& data) const { return decrypt(data); }
};

}  // namespace librevault::crypto
