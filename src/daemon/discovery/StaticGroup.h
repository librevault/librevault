#pragma once
#include <QTimer>

#include "DiscoveryResult.h"

namespace librevault {

class FolderGroup;
class StaticGroup : public QObject {
  Q_OBJECT
 public:
  StaticGroup(FolderGroup* fgroup);
  virtual ~StaticGroup() = default;

  void setEnabled(bool enabled);

 signals:
  void discovered(DiscoveryResult result);

 private:
  FolderGroup* fgroup_;
  QTimer* timer_;

 private slots:
  void tick();
};

}  // namespace librevault
