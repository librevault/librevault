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
#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_hiersvc, "hiersvc")

HierarchicalService::HierarchicalService(HierarchicalService* parent_svc, QObject* parent)
    : QObject(parent), parent_svc_(parent_svc), root_(!parent_svc) {
  connect(this, &HierarchicalService::started, this, [this] { setState(State::STARTED); });
  if (parent_svc) {
    connect(parent_svc, &HierarchicalService::started, this, &HierarchicalService::tryStart);
    connect(parent_svc, &HierarchicalService::stopChildren, this,
        [this] { setState(State::DISABLED); });
  }
}

void HierarchicalService::setEnabled(bool enabled) {
  qCDebug(log_hiersvc) << this << "setEnabled:" << enabled;
  if (!isEnabled() && enabled)
    setState(State::ENABLED);
  else if (isEnabled() && !enabled)
    setState(State::DISABLED);
}

bool HierarchicalService::isParentOperational() const {
  return root_ ? true : parent_svc_ && parent_svc_->isOperational();
}

void HierarchicalService::tryStart() {
  if (state_ == State::ENABLED && isParentOperational()) setState(State::STARTING);
}

void HierarchicalService::teardown(State old_state) {
  if (old_state == State::STARTED) stopChildren();
  if (old_state == State::STARTING || old_state == State::STARTED) {
    stop();
    stopped();
  }
}

void HierarchicalService::setState(State new_state) {
  State old_state = state_;
  qCDebug(log_hiersvc) << this << "state transition" << old_state << "-->" << new_state;
  if (state_ == new_state) return;
  state_ = new_state;

  switch (new_state) {
    case State::DISABLED: teardown(old_state); break;
    case State::ENABLED:
      teardown(old_state);
      tryStart();
      break;
    case State::STARTING:
      Q_ASSERT(old_state != State::DISABLED && old_state != State::STARTED);
      start();
      break;
    case State::STARTED:
      Q_ASSERT(old_state != State::DISABLED && old_state != State::ENABLED);
      break;
  }
}

} /* namespace librevault */
