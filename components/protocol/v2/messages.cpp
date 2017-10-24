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
#include <messages_v2.pb.h>
#include <QDataStream>

namespace librevault {
namespace protocol {
namespace v2 {

#pragma mark Debug output
QDebug operator<<(QDebug debug, const Message::Header::MessageType& obj) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "MessageType(" << [=] {
    switch (obj) {
      case Message::Header::MessageType::HANDSHAKE: return "HANDSHAKE";
      case Message::Header::MessageType::CHOKE: return "CHOKE";
      case Message::Header::MessageType::UNCHOKE: return "UNCHOKE";
      case Message::Header::MessageType::INTEREST: return "INTEREST";
      case Message::Header::MessageType::UNINTEREST: return "UNINTEREST";
      case Message::Header::MessageType::INDEXUPDATE: return "INDEXUPDATE";
      case Message::Header::MessageType::METAREQUEST: return "METAREQUEST";
      case Message::Header::MessageType::METARESPONSE: return "METARESPONSE";
      case Message::Header::MessageType::BLOCKREQUEST: return "BLOCKREQUEST";
      case Message::Header::MessageType::BLOCKRESPONSE: return "BLOCKRESPONSE";
      default: return "UNKNOWN";
    }
  }() << ")";
  return debug;
}

QDebug operator<<(QDebug debug, const Message::Header& obj) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "Header(" << obj.type << ")";
  return debug;
}

QDebug operator<<(QDebug debug, const Message::Handshake& obj) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "Handshake("
                  << "auth_token: " << obj.auth_token.toHex() << ", device_name: " << obj.device_name
                  << ", user_agent: " << obj.user_agent << ", dht_port: " << obj.dht_port << ")";
  return debug;
}

QDebug operator<<(QDebug debug, const Message::IndexUpdate& obj) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "IndexUpdate("
                  << "path_keyed_hash: " << obj.revision.path_keyed_hash_.toHex()
                  << ", revision: " << obj.revision.revision_ << ", bitfield: " << obj.bitfield
                  << ")";
  return debug;
}

QDebug operator<<(QDebug debug, const Message::MetaRequest& obj) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "MetaRequest("
                  << "path_keyed_hash: " << obj.revision.path_keyed_hash_.toHex()
                  << ", revision: " << obj.revision.revision_ << ")";
  return debug;
}

QDebug operator<<(QDebug debug, const Message::MetaResponse& obj) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "MetaResponse("
                  << "smeta: " << obj.smeta << ", bitfield: " << obj.bitfield << ")";
  return debug;
}

QDebug operator<<(QDebug debug, const Message::BlockRequest& obj) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "BlockRequest("
                  << "ct_hash: " << obj.ct_hash.toHex() << ", offset: " << obj.offset
                  << ", length: " << obj.length << ")";
  return debug;
}

QDebug operator<<(QDebug debug, const Message::BlockResponse& obj) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "BlockResponse("
                  << "ct_hash: " << obj.ct_hash.toHex() << ", offset: " << obj.offset << ", content: "
                  << "(actual content size: " + QString::number(obj.content.size()) + ")"
                  << ")";
  return debug;
}

QDebug operator<<(QDebug debug, const Message& obj) {
  QDebugStateSaver saver(debug);
  debug.nospace();
  debug << "Message(";
  debug << obj.header;

  if (obj.header.type != Message::Header::MessageType::CHOKE &&
      obj.header.type != Message::Header::MessageType::UNCHOKE &&
      obj.header.type != Message::Header::MessageType::INTEREST &&
      obj.header.type != Message::Header::MessageType::UNINTEREST) {
    debug << ", ";
  }

  switch (obj.header.type) {
    case Message::Header::MessageType::HANDSHAKE: debug << obj.handshake; break;
    case Message::Header::MessageType::INDEXUPDATE: debug << obj.indexupdate; break;
    case Message::Header::MessageType::METAREQUEST: debug << obj.metarequest; break;
    case Message::Header::MessageType::METARESPONSE: debug << obj.metaresponse; break;
    case Message::Header::MessageType::BLOCKREQUEST: debug << obj.blockrequest; break;
    case Message::Header::MessageType::BLOCKRESPONSE: debug << obj.blockresponse; break;
    default:;
  }
  debug << ")";
  return debug;
}

}  // namespace v2
}  // namespace protocol
}  // namespace librevault
