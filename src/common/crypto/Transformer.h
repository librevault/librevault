#pragma once

#include <QByteArray>

namespace librevault::crypto {

class OneWayTransformer {
 public:
  virtual ~OneWayTransformer() = default;

  virtual QByteArray to(const QByteArray& data) const = 0;

  template <class InputIterator>
  QByteArray to(InputIterator first, InputIterator last) const {
    return to(QByteArray::fromStdString({first, last}));
  }
  template <class Container>
  QByteArray to(const Container& data) const {
    return to(data.begin(), data.end());
  }
};

class TwoWayTransformer : public OneWayTransformer {
 public:
  ~TwoWayTransformer() override = default;

  QByteArray to(const QByteArray& data) const override = 0;
  virtual QByteArray from(const QByteArray& data) const = 0;

  template <class InputIterator>
  QByteArray from(InputIterator first, InputIterator last) const {
    return from(QByteArray::fromStdString({first, last}));
  }
  template <class Container>
  QByteArray from(const Container& data) const {
    return from(data.begin(), data.end());
  }
};

template <class Trans>
class De : public TwoWayTransformer {
  Trans nested;

 public:
  template <class... Args>
  explicit De(Args... trans_args) : nested(trans_args...) {}

  QByteArray to(const QByteArray& data) const override { return nested.from(data); };
  QByteArray from(const QByteArray& data) const override { return nested.to(data); };
};

inline QByteArray operator|(const QByteArray& data, OneWayTransformer&& transformer) { return transformer.to(data); }

template <class Container>
inline QByteArray operator|(const Container& data, OneWayTransformer&& transformer) {
  return transformer.to(data.begin(), data.end());
}

}  // namespace librevault::crypto
