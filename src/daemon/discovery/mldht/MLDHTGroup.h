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
#pragma once
#include <QTimer>

#include "discovery/DiscoveryResult.h"
#include "discovery/btcompat.h"

namespace librevault {

class MLDHTProvider;
class FolderGroup;

class MLDHTGroup : public QObject {
  Q_OBJECT
 public:
  MLDHTGroup(MLDHTProvider* provider, FolderGroup* fgroup);

  void setEnabled(bool enable);
  void startSearch(int af);

 signals:
  void discovered(DiscoveryResult result);

 public slots:
  void handleEvent(int event, btcompat::info_hash ih, QByteArray values);

 private:
  MLDHTProvider* provider_;
  QTimer* timer_;

  btcompat::info_hash info_hash_;
  QByteArray folderid_;

  bool enabled_ = false;
};

}  // namespace librevault
