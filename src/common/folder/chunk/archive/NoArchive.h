#pragma once
#include "Archive.h"

namespace librevault {

class NoArchive : public ArchiveStrategy {
  Q_OBJECT
 public:
  NoArchive(Archive* parent) : ArchiveStrategy(parent) {}
  void archive(const QString& denormpath) override;
};

}  // namespace librevault
