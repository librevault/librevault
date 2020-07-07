#pragma once
#include <QTimer>

#include "DiscoveryResult.h"

namespace librevault {

class FolderGroup;
class StaticGroup : public QObject {
  Q_OBJECT
 public:
  StaticGroup(FolderGroup* fgroup, QObject* parent);
  ~StaticGroup() override = default;

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
