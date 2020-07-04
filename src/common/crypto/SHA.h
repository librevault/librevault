#pragma once
#include <QtCore/QCryptographicHash>

#include "Transformer.h"

namespace librevault::crypto {

class SHA2 : public OneWayTransformer {
  const size_t size;

 public:
  explicit SHA2(size_t size) : size(size) {}
  ~SHA2() override = default;

  [[nodiscard]] QByteArray to(const QByteArray& data) const override {
    switch (size) {
      case 224:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha224);
      case 256:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
      case 384:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha384);
      case 512:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha512);
      default:
        throw std::runtime_error("Hashing algorithm not supported");
    }
  }
};

class SHA3 : public OneWayTransformer {
  const size_t size_;

 public:
  explicit SHA3(size_t size) : size_(size) {}
  ~SHA3() override = default;

  [[nodiscard]] QByteArray to(const QByteArray& data) const override {
    switch (size_) {
      case 224:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha3_224);
      case 256:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha3_256);
      case 384:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha3_384);
      case 512:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha3_512);
      default:
        throw std::runtime_error("Hashing algorithm not supported");
    }
  }
};

}  // namespace librevault::crypto
