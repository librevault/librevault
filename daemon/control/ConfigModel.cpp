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
#include "ConfigModel.h"
#include <QJsonArray>

namespace librevault {

ConfigModel::ConfigModel(const QJsonObject& fconfig) {
  // Necessary
  client_name = fconfig["client_name"].toString();
  control_listen = fconfig["control_listen"].toInt();
  p2p_listen = fconfig["p2p_listen"].toInt();
  p2p_download_slots = fconfig["p2p_download_slots"].toInt();
  p2p_request_timeout = std::chrono::seconds(fconfig["p2p_request_timeout"].toInt());
  p2p_block_size = fconfig["p2p_block_size"].toInt();
  natpmp_enabled = fconfig["natpmp_enabled"].toBool();
  natpmp_lifetime = std::chrono::seconds(fconfig["natpmp_lifetime"].toInt());
  upnp_enabled = fconfig["upnp_enabled"].toBool();
  predef_repeat_interval = std::chrono::seconds(fconfig["predef_repeat_interval"].toInt());
  multicast_enabled = fconfig["multicast_enabled"].toBool();
  multicast_repeat_interval = std::chrono::seconds(fconfig["multicast_repeat_interval"].toInt());
  bttracker_enabled = fconfig["bttracker_enabled"].toBool();
  bttracker_num_want = fconfig["bttracker_num_want"].toInt();
  bttracker_min_interval = std::chrono::seconds(fconfig["bttracker_min_interval"].toInt());
  bttracker_azureus_id = fconfig["bttracker_azureus_id"].toString();
  bttracker_reconnect_interval =
      std::chrono::seconds(fconfig["bttracker_reconnect_interval"].toInt());
  bttracker_packet_timeout = std::chrono::seconds(fconfig["bttracker_packet_timeout"].toInt());
  mainline_dht_enabled = fconfig["mainline_dht_enabled"].toBool();
  mainline_dht_port = fconfig["mainline_dht_port"].toInt();
  for (const auto& router : fconfig["mainline_dht_routers"].toArray())
    mainline_dht_routers << router.toString();
}

}  // namespace librevault
