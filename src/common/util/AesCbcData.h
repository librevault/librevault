#pragma once
#include "Secret.h"

namespace librevault {

class AesCbcData {
 public:
  QByteArray ct() const;
  QByteArray iv() const;

  void setEncrypted(const std::string& ct, const std::string& iv);

  bool check() const { return !ct_.isEmpty() && ct_.size() % 16 == 0 && iv_.size() == 16; };
  // Use this with extreme care. Can cause padding oracle attack, if misused. Meta is
  // (generally) signed and unmalleable
  void set_plain(const QByteArray& pt, const Secret& secret);
  QByteArray get_plain(const Secret& secret) const;  // Caching, maybe?
 private:
  QByteArray ct_, iv_;
};

}  // namespace librevault