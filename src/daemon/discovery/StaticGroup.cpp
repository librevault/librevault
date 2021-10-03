/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "StaticGroup.h"

#include "control/FolderParams.h"
#include "folder/FolderGroup.h"

namespace librevault {

StaticGroup::StaticGroup(FolderGroup* fgroup) : fgroup_(fgroup) {
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
  QString source = QStringLiteral("Static");
  for (const QUrl& node : fgroup_->params().nodes) {
    DiscoveryResult result;
    result.source = source;
    result.url = node;
    emit discovered(result);
  }
}

}  // namespace librevault
