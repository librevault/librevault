/* Copyright (C) 2017 Alexander Shishenko <alex@shishenko.com>
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
#include "MessageHandler.h"
#include "Parser.h"
#include <QDebug>

namespace librevault {

MessageHandler::MessageHandler(QObject* parent) :
	QObject(parent) {
}

void MessageHandler::sendChoke() {
	emit messagePrepared(protocol::v2::Parser().genChoke());
	qDebug() << "==> CHOKE";
}
void MessageHandler::sendUnchoke() {
	emit messagePrepared(protocol::v2::Parser().genUnchoke());
	qDebug() << "==> UNCHOKE";
}
void MessageHandler::sendInterested() {
	emit messagePrepared(protocol::v2::Parser().genInterested());
	qDebug() << "==> INTERESTED";
}
void MessageHandler::sendNotInterested() {
	emit messagePrepared(protocol::v2::Parser().genNotInterested());
	qDebug() << "==> NOT_INTERESTED";
}

void MessageHandler::sendHaveMeta(const MetaInfo::PathRevision& revision, QBitArray bitfield) {
	protocol::v2::IndexUpdate message;
	message.revision = revision;
	message.bitfield = bitfield;
	emit messagePrepared(protocol::v2::Parser().serializeIndexUpdate(message));

	qDebug() << "==> HAVE_META:"
		<< "path_id=" << message.revision.path_keyed_hash_.toHex()
		<< "revision=" << message.revision.revision_
		<< "bits=" << message.bitfield;
}

void MessageHandler::sendMetaRequest(const MetaInfo::PathRevision& revision) {
	protocol::v2::MetaRequest message;
	message.revision = revision;
	emit messagePrepared(protocol::v2::Parser().serializeMetaRequest(message));

	qDebug() << "==> META_REQUEST:"
		<< "path_id=" << revision.path_keyed_hash_.toHex()
		<< "revision=" << revision.revision_;
}
void MessageHandler::sendMetaReply(const SignedMeta& smeta, QBitArray bitfield) {
	protocol::v2::MetaResponse message;
	message.smeta = smeta;
	message.bitfield = bitfield;
	emit messagePrepared(protocol::v2::Parser().serializeMetaReply(message));

	qDebug() << "==> META_REPLY:"
		<< "path_id=" << smeta.metaInfo().pathKeyedHash().toHex()
		<< "revision=" << smeta.metaInfo().timestamp().time_since_epoch().count()
		<< "bits=" << bitfield;
}

void MessageHandler::sendBlockRequest(QByteArray ct_hash, uint32_t offset, uint32_t length) {
	protocol::v2::BlockRequest message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	emit messagePrepared(protocol::v2::Parser().serializeBlockRequest(message));

	qDebug() << "==> BLOCK_REQUEST:"
		<< "ct_hash=" << ct_hash.toHex()
		<< "offset=" << offset
		<< "length=" << length;
}
void MessageHandler::sendBlockReply(QByteArray ct_hash, uint32_t offset, QByteArray block) {
	protocol::v2::BlockResponse message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.content = block;
	emit messagePrepared(protocol::v2::Parser().serializeBlockResponse(message));

	qDebug() << "==> BLOCK_REPLY:"
		<< "ct_hash=" << ct_hash.toHex()
		<< "offset=" << offset;
}

void MessageHandler::handleChoke(QByteArray message_raw) {
	qDebug() << "<== CHOKE";
	emit rcvdChoke();
}
void MessageHandler::handleUnchoke(QByteArray message_raw) {
	qDebug() << "<== UNCHOKE";
	emit rcvdUnchoke();
}
void MessageHandler::handleInterested(QByteArray message_raw) {
	qDebug() << "<== INTERESTED";
	emit rcvdInterested();
}
void MessageHandler::handleNotInterested(QByteArray message_raw) {
	qDebug() << "<== NOT_INTERESTED";
	emit rcvdNotInterested();
}

void MessageHandler::handleHaveMeta(QByteArray message_raw) {
	auto message_struct = protocol::v2::Parser().parseIndexUpdate(message_raw);
	qDebug() << "<== HAVE_META:"
		<< "path_id=" << message_struct.revision.path_keyed_hash_.toHex()
		<< "revision=" << message_struct.revision.revision_
		<< "bits=" << message_struct.bitfield;

	emit rcvdHaveMeta(message_struct.revision, message_struct.bitfield);
}

void MessageHandler::handleMetaRequest(QByteArray message_raw) {
	auto message_struct = protocol::v2::Parser().parseMetaRequest(message_raw);
	qDebug() << "<== META_REQUEST:"
		<< "path_id=" << message_struct.revision.path_keyed_hash_.toHex()
		<< "revision=" << message_struct.revision.revision_;

	emit rcvdMetaRequest(message_struct.revision);
}
void MessageHandler::handleMetaReply(QByteArray message_raw) {
	auto message_struct = protocol::v2::Parser().parseMetaReply(message_raw);
	qDebug() << "<== META_REPLY:"
		<< "path_id=" << message_struct.smeta.metaInfo().pathKeyedHash().toHex()
		<< "revision=" << message_struct.smeta.metaInfo().revision()
		<< "bits=" << message_struct.bitfield;

	emit rcvdMetaReply(message_struct.smeta, message_struct.bitfield);
}

void MessageHandler::handleBlockRequest(QByteArray message_raw) {
	auto message_struct = protocol::v2::Parser().parseBlockRequest(message_raw);
	qDebug() << "<== BLOCK_REQUEST:"
		<< "ct_hash=" << message_struct.ct_hash.toHex()
		<< "length=" << message_struct.length
		<< "offset=" << message_struct.offset;

	emit rcvdBlockRequest(message_struct.ct_hash, message_struct.offset, message_struct.length);
}
void MessageHandler::handleBlockReply(QByteArray message_raw) {
	auto message_struct = protocol::v2::Parser().parseBlockResponse(message_raw);
	qDebug() << "<== BLOCK_REPLY:"
		<< "ct_hash=" << message_struct.ct_hash.toHex()
		<< "offset=" << message_struct.offset;

	emit rcvdBlockReply(message_struct.ct_hash, message_struct.offset, message_struct.content);
}

} /* namespace librevault */
