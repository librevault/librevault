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
#include <QJsonObject>
#include <QList>
#include <QString>
#include <chrono>

namespace librevault {

struct ConfigModel {
  explicit ConfigModel(const QJsonObject& fconfig);

  // Parameters
  QString client_name;
  quint16 control_listen;
  quint16 p2p_listen;
  qint16 p2p_download_slots;
  std::chrono::seconds p2p_request_timeout;
  quint32 p2p_block_size;
  bool natpmp_enabled;
  std::chrono::seconds natpmp_lifetime;
  bool upnp_enabled;
  std::chrono::seconds predef_repeat_interval;
  bool multicast_enabled;
  std::chrono::seconds multicast_repeat_interval;
  bool bttracker_enabled;
  quint16 bttracker_num_want;
  std::chrono::seconds bttracker_min_interval;
  QString bttracker_azureus_id;
  std::chrono::seconds bttracker_reconnect_interval;
  std::chrono::seconds bttracker_packet_timeout;
  bool mainline_dht_enabled;
  quint16 mainline_dht_port;
  QList<QString> mainline_dht_routers;
};

}  // namespace librevault
