#pragma once
#include "Transformer.h"

namespace librevault::crypto {

namespace alphabet {
static QByteArray bitcoin = QByteArrayLiteral("123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz");
}  // namespace alphabet

class Base58 : public TwoWayTransformer {
 private:
  QByteArray current_alphabet;

 public:
  explicit Base58(QByteArray alphabet = alphabet::bitcoin);
  QByteArray to(const QByteArray& data) const override;
  QByteArray from(const QByteArray& data) const override;
};

}  // namespace librevault::crypto
