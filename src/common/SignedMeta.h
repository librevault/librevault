#pragma once
#include <memory>
#include <optional>

#include "Meta.h"

namespace librevault {

class SignedMeta {
 public:
  struct SignatureError : Meta::error {
    SignatureError() : Meta::error("Meta signature mismatch") {}
  };

  SignedMeta() = default;
  SignedMeta(Meta meta, const Secret& secret);
  SignedMeta(const QByteArray& raw_meta, QByteArray signature, const Secret& secret, bool check_signature = true);

  operator bool() const { return meta_ && !raw_meta_.isEmpty() && !signature_.isEmpty(); }

  // Getters
  const Meta& meta() const { return *meta_; }
  const QByteArray& raw_meta() const { return raw_meta_; }
  const QByteArray& signature() const { return signature_; }

 private:
  std::optional<Meta> meta_;

  QByteArray raw_meta_;
  QByteArray signature_;
};

}  // namespace librevault
