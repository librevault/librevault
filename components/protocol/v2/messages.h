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
#pragma once

#include <SignedMeta.h>
#include <QBitArray>
#include <QDebug>
#include <QString>
#include <QVariantMap>

namespace librevault {
namespace protocol {
namespace v2 {

enum MessageType : quint8 {
  HANDSHAKE = 0,

  CHOKE = 1,
  UNCHOKE = 2,
  INTEREST = 3,
  UNINTEREST = 4,

  INDEXUPDATE = 5,

  METAREQUEST = 6,
  METARESPONSE = 7,

  BLOCKREQUEST = 8,
  BLOCKRESPONSE = 9,
};

struct Header {
  MessageType type;
};

struct Handshake {
  QByteArray auth_token;
  QString device_name;
  QString user_agent;
  quint16 dht_port;
};
struct IndexUpdate {
  MetaInfo::PathRevision revision;
  QBitArray bitfield;
};
struct MetaRequest {
  MetaInfo::PathRevision revision;
};
struct MetaResponse {
  SignedMeta smeta;
  QBitArray bitfield;
};
struct BlockRequest {
  QByteArray ct_hash;
  quint32 offset;
  quint32 length;
};
struct BlockResponse {
  QByteArray ct_hash;
  quint32 offset;
  QByteArray content;
};

QDebug operator<<(QDebug debug, Header message_struct);
QDebug operator<<(QDebug debug, Handshake message_struct);
QDebug operator<<(QDebug debug, IndexUpdate message_struct);
QDebug operator<<(QDebug debug, MetaRequest message_struct);
QDebug operator<<(QDebug debug, MetaResponse message_struct);
QDebug operator<<(QDebug debug, BlockRequest message_struct);
QDebug operator<<(QDebug debug, BlockResponse message_struct);

}  // namespace v2
}  // namespace protocol
}  // namespace librevault
