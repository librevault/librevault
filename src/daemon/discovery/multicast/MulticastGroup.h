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
#include <util/Endpoint.h>

#include <QTimer>
#include <QUdpSocket>

namespace librevault {

class FolderGroup;
class MulticastProvider;
class MulticastGroup : public QObject {
  Q_OBJECT

 public:
  MulticastGroup(MulticastProvider* provider, FolderGroup* fgroup);

  void setEnabled(bool enabled);

 private:
  MulticastProvider* provider_;
  FolderGroup* fgroup_;

  QTimer* timer_;
  bool enabled_ = false;

  QByteArray message_;

  QByteArray getMessage();
  void sendMulticast(QUdpSocket* socket, const Endpoint& endpoint);

 private slots:
  void sendMulticasts();
};

}  // namespace librevault
