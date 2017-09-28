/* Copyright (C) 2014-2017 Alexander Shishenko <alex@shishenko.com>
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
#include "messages.h"

namespace librevault {
namespace protocol {
namespace v2 {

QDebug operator<<(QDebug debug, Header message_struct) {
  return debug << QVariantMap{
      {"type", message_struct.type},
  };
}
QDebug operator<<(QDebug debug, Handshake message_struct) {
  return debug << QVariantMap{
      {"auth_token", message_struct.auth_token},
      {"device_name", message_struct.device_name},
      {"user_agent", message_struct.user_agent},
      {"dht_port", message_struct.dht_port},
  };
}
QDebug operator<<(QDebug debug, IndexUpdate message_struct) {
  return debug << QVariantMap{
      {"path_keyed_hash", message_struct.revision.path_keyed_hash_},
      {"revision", message_struct.revision.revision_},
      {"bitfield", message_struct.bitfield},
  };
}
QDebug operator<<(QDebug debug, MetaRequest message_struct) {
  return debug << QVariantMap{
      {"path_keyed_hash", message_struct.revision.path_keyed_hash_},
      {"revision", message_struct.revision.revision_},
  };
}
QDebug operator<<(QDebug debug, MetaResponse message_struct) {
  return debug << QVariantMap{
      {"metainfo", message_struct.smeta.rawMetaInfo()},
      {"signature", message_struct.smeta.signature()},
      {"bitfield", message_struct.bitfield},
  };
}
QDebug operator<<(QDebug debug, BlockRequest message_struct) {
  return debug << QVariantMap{
      {"ct_hash", message_struct.ct_hash},
      {"offset", message_struct.offset},
      {"length", message_struct.length},
  };
}
QDebug operator<<(QDebug debug, BlockResponse message_struct) {
  return debug << QVariantMap{
      {"ct_hash", message_struct.ct_hash},
      {"offset", message_struct.offset},
      {"content", "(actual content size: " + QString::number(message_struct.content.size()) + ")"},
  };
}

}  // namespace v2
}  // namespace protocol
}  // namespace librevault
