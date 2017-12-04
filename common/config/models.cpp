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
#include "models.h"
#include <QJsonArray>
#include <QVariant>

namespace librevault {

template <class Value>
void packValue(QJsonValueRef j, const Value& val) {
  j = QJsonValue::fromVariant(val);
}

template <>
void packValue(QJsonValueRef j, const std::chrono::seconds& val) {
  j = std::chrono::duration<double>(val).count();
}

template <>
void packValue(QJsonValueRef j, const Secret& val) {
  j = val.toString();
}

template <>
void packValue(QJsonValueRef j, const QList<QUrl>& val) {
  QStringList strlist;
  for (const auto& url : val) strlist << url.toString();
  j = QJsonArray::fromStringList(strlist);
}

template <class Value>
void unpackValue(Value& val, const QJsonValue& j) {
  val = qvariant_cast<Value>(j.toVariant());
}

template <>
void unpackValue(std::chrono::seconds& val, const QJsonValue& j) {
  val =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::duration<double>(j.toDouble()));
}

template <>
void unpackValue(Secret& val, const QJsonValue& j) {
  val = Secret(j.toString());
}

}  // namespace librevault

namespace librevault::models {

QJsonObject ClientSettings::toJson() const {
  QJsonObject j;

  packValue(j["client_name"], client_name);
  packValue(j["control_listen"], control_listen);
  packValue(j["p2p_listen"], p2p_listen);
  packValue(j["p2p_download_slots"], p2p_download_slots);
  packValue(j["p2p_request_timeout"], p2p_request_timeout);
  packValue(j["p2p_block_size"], (int)p2p_block_size);
  packValue(j["natpmp_enabled"], natpmp_enabled);
  packValue(j["natpmp_lifetime"], natpmp_lifetime);
  packValue(j["upnp_enabled"], upnp_enabled);
  packValue(j["predef_repeat_interval"], predef_repeat_interval);
  packValue(j["multicast_enabled"], multicast_enabled);
  packValue(j["multicast_repeat_interval"], multicast_repeat_interval);
  packValue(j["bttracker_enabled"], bttracker_enabled);
  packValue(j["bttracker_num_want"], bttracker_num_want);
  packValue(j["bttracker_min_interval"], bttracker_min_interval);
  packValue(j["bttracker_azureus_id"], bttracker_azureus_id);
  packValue(j["bttracker_reconnect_interval"], bttracker_reconnect_interval);
  packValue(j["bttracker_packet_timeout"], bttracker_packet_timeout);
  packValue(j["mainline_dht_enabled"], mainline_dht_enabled);
  packValue(j["mainline_dht_port"], mainline_dht_port);
  packValue(j["mainline_dht_routers"], mainline_dht_routers);

  return j;
}

ClientSettings ClientSettings::fromJson(const QJsonObject& j) {
  ClientSettings st{};

  unpackValue(st.client_name, j["client_name"]);
  unpackValue(st.control_listen, j["control_listen"]);
  unpackValue(st.p2p_listen, j["p2p_listen"]);
  unpackValue(st.p2p_download_slots, j["p2p_download_slots"]);
  unpackValue(st.p2p_request_timeout, j["p2p_request_timeout"]);
  unpackValue(st.p2p_block_size, j["p2p_block_size"]);
  unpackValue(st.natpmp_enabled, j["natpmp_enabled"]);
  unpackValue(st.natpmp_lifetime, j["natpmp_lifetime"]);
  unpackValue(st.upnp_enabled, j["upnp_enabled"]);
  unpackValue(st.predef_repeat_interval, j["predef_repeat_interval"]);
  unpackValue(st.multicast_enabled, j["multicast_enabled"]);
  unpackValue(st.multicast_repeat_interval, j["multicast_repeat_interval"]);
  unpackValue(st.bttracker_enabled, j["bttracker_enabled"]);
  unpackValue(st.bttracker_num_want, j["bttracker_num_want"]);
  unpackValue(st.bttracker_min_interval, j["bttracker_min_interval"]);
  unpackValue(st.bttracker_azureus_id, j["bttracker_azureus_id"]);
  unpackValue(st.bttracker_reconnect_interval, j["bttracker_reconnect_interval"]);
  unpackValue(st.bttracker_packet_timeout, j["bttracker_packet_timeout"]);
  unpackValue(st.mainline_dht_enabled, j["mainline_dht_enabled"]);
  unpackValue(st.mainline_dht_port, j["mainline_dht_port"]);
  unpackValue(st.mainline_dht_routers, j["mainline_dht_routers"]);

  return st;
}

}  // namespace librevault::models

namespace librevault {

FolderSettings::FolderSettings(const QJsonObject& doc) { fromJson(doc); }

QJsonObject FolderSettings::toJson() const {
  QJsonObject j;

  packValue(j["secret"], secret);
  packValue(j["path"], path);
  packValue(j["system_path"], system_path);
  packValue(j["preserve_unix_attrib"], preserve_unix_attrib);
  packValue(j["preserve_windows_attrib"], preserve_windows_attrib);
  packValue(j["preserve_symlinks"], preserve_symlinks);
  packValue(j["full_rescan_interval"], full_rescan_interval);
  packValue(j["nodes"], nodes);
  packValue(j["mainline_dht_enabled"], mainline_dht_enabled);

  return j;
}

FolderSettings FolderSettings::fromJson(const QJsonObject& j) {
  FolderSettings st{QJsonObject()};

  unpackValue(st.secret, j["secret"]);
  unpackValue(st.path, j["path"]);
  unpackValue(st.system_path, j["system_path"]);
  unpackValue(st.preserve_unix_attrib, j["preserve_unix_attrib"]);
  unpackValue(st.preserve_windows_attrib, j["preserve_windows_attrib"]);
  unpackValue(st.preserve_symlinks, j["preserve_symlinks"]);
  unpackValue(st.full_rescan_interval, j["full_rescan_interval"]);
  unpackValue(st.nodes, j["nodes"]);
  unpackValue(st.mainline_dht_enabled, j["mainline_dht_enabled"]);

  return st;
}

}  // namespace librevault
