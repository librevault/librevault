#pragma once
#include "Transformer.h"

namespace librevault::crypto {

/**
 * This transformation is not really HMAC, as HMAC(Keccak) is not supported by CryptoPP.
 * As described on http://keccak.noekeon.org/ (than's Keccak creators' site):
 *
 *   Unlike SHA-1 and SHA-2, Keccak does not have the length-extension weakness,
 *   hence does not need the HMAC nested construction. Instead, MAC computation
 *   can be performed by simply prepending the message with the key.
 *
 * Okay, this solution is implemented here. Just prepending key to message.
 */
class KMAC_SHA3_224 : public OneWayTransformer {
  const QByteArray key_;

 public:
  explicit KMAC_SHA3_224(QByteArray key) : key_(std::move(key)) {}
  ~KMAC_SHA3_224() override = default;

  [[nodiscard]] QByteArray compute(const QByteArray& data) const;
  [[nodiscard]] QByteArray to(const QByteArray& data) const override;
};

}  // namespace librevault::crypto
