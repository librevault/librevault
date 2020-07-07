#pragma once
#include <QByteArray>
#include <array>

namespace librevault::btcompat {

using info_hash = std::array<uint8_t, 20>;
using peer_id = std::array<uint8_t, 20>;

inline info_hash getInfoHash(const QByteArray& folderid) {
  info_hash ih;
  std::copy(folderid.begin(), folderid.begin() + std::min(ih.size(), (size_t)folderid.size()), ih.begin());
  return ih;
}

}  // namespace librevault::btcompat
