#pragma once
#include "Transformer.h"

namespace librevault::crypto {

class Base32 : public TwoWayTransformer {
 public:
  QByteArray to(const QByteArray& data) const override;
  QByteArray from(const QByteArray& data) const override;
};

}  // namespace librevault::crypto
