#include "StaticGroup.h"

#include "control/FolderParams.h"
#include "folder/FolderGroup.h"

namespace librevault {

StaticGroup::StaticGroup(FolderGroup* fgroup, QObject* parent) : QObject(parent), fgroup_(fgroup) {
  timer_ = new QTimer(this);
  timer_->setInterval(30 * 1000);
  QTimer::singleShot(0, this, &StaticGroup::tick);
  connect(timer_, &QTimer::timeout, this, &StaticGroup::tick);
}

void StaticGroup::setEnabled(bool enabled) {
  if (!timer_->isActive() && enabled)
    timer_->start();
  else if (timer_->isActive() && !enabled)
    timer_->stop();
}

void StaticGroup::tick() {
  auto source = QStringLiteral("Static");
  for (const QUrl& node : fgroup_->params().nodes) {
    DiscoveryResult result;
    result.source = source;
    result.url = node;
    emit discovered(result);
  }
}

}  // namespace librevault
