#pragma once
#include "Transformer.h"

namespace librevault::crypto {

namespace alphabet {
static std::string bitcoin = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
}  // namespace alphabet

class Base58 : public TwoWayTransformer {
 private:
  const std::string& current_alphabet;

 public:
  explicit Base58(const std::string& alphabet = alphabet::bitcoin);
  QByteArray to(const QByteArray& data) const override;
  QByteArray from(const QByteArray& data) const override;
};

}  // namespace librevault::crypto
