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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "HierarchicalService.h"

namespace librevault {

HierarchicalService::HierarchicalService(HierarchicalService* parent_svc, QObject* parent)
    : QObject(parent), parent_svc_(parent_svc), root_(!parent_svc) {
  connect(this, &HierarchicalService::started, this, &HierarchicalService::start);
  connect(this, &HierarchicalService::stopped, this, &HierarchicalService::stop);

  if (parent_svc) {
    connect(parent_svc, &HierarchicalService::started, this, [this] {
      if (isOperational()) emit started();
    });
    connect(parent_svc, &HierarchicalService::stopped, this, &HierarchicalService::stopped);
  }
}

void HierarchicalService::setEnabled(bool enabled) {
  if (enabled_ == enabled) return;

  enabled_ = enabled;
  if (isOperational())
    emit started();
  else if (!isEnabled() && isParentOperational())
    emit stopped();
}

bool HierarchicalService::isOperational() const { return isEnabled() && isParentOperational(); }

bool HierarchicalService::isParentOperational() const {
  return root_ ? true : parent_svc_ && parent_svc_->isOperational();
}

} /* namespace librevault */
