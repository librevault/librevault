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
#pragma once
#include <QObject>
#include <QPointer>

namespace librevault {

class HierarchicalService : public QObject {
  Q_OBJECT

 public:
  enum class State {
    DISABLED = 0,
    ENABLED = 1,
    STARTING = 2,
    STARTED = 3
  } state_ = State::DISABLED;
  Q_ENUM(State);

  explicit HierarchicalService(
      HierarchicalService* parent_svc = nullptr, QObject* parent = nullptr);
  HierarchicalService(const HierarchicalService&) = delete;
  HierarchicalService(HierarchicalService&&) = delete;
  ~HierarchicalService() { setState(State::DISABLED); }

  bool isEnabled() const { return state_ != State::DISABLED; }
  void setEnabled(bool enabled);

  Q_SIGNAL void started();
  Q_SIGNAL void stopped();

 protected:
  QPointer<HierarchicalService> parent_svc_;

  virtual void start() { emit started(); }
  virtual void stop() {}

  bool isOperational() const { return state_ == State::STARTED; }
  bool isParentOperational() const;

 private:
  const bool root_ = true;

  Q_SIGNAL void stopChildren();
  void tryStart();
  void teardown(State old_state);

  void setState(State new_state);
};

} /* namespace librevault */
