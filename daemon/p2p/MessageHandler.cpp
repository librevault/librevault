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
#include "util/readable.h"
#include "conv_bitarray.h"
#include <librevault/protocol/V1Parser.h>
#include <QDebug>

namespace librevault {

MessageHandler::MessageHandler(QObject* parent) :
	QObject(parent) {
}

void MessageHandler::sendChoke() {
	emit messagePrepared(V1Parser().gen_Choke());
	qDebug() << "==> CHOKE";
}
void MessageHandler::sendUnchoke() {
	emit messagePrepared(V1Parser().gen_Unchoke());
	qDebug() << "==> UNCHOKE";
}
void MessageHandler::sendInterested() {
	emit messagePrepared(V1Parser().gen_Interested());
	qDebug() << "==> INTERESTED";
}
void MessageHandler::sendNotInterested() {
	emit messagePrepared(V1Parser().gen_NotInterested());
	qDebug() << "==> NOT_INTERESTED";
}

void MessageHandler::sendHaveMeta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	V1Parser::HaveMeta message;
	message.revision = revision;
	message.bitfield = bitfield;
	emit messagePrepared(V1Parser().gen_HaveMeta(message));

	qDebug() << "==> HAVE_META:"
		<< "path_id=" << path_id_readable(message.revision.path_id_)
		<< "revision=" << message.revision.revision_
		<< "bits=" << conv_bitarray(message.bitfield);
}
void MessageHandler::sendHaveChunk(QByteArray ct_hash) {
	V1Parser::HaveChunk message;
	message.ct_hash = conv_bytearray(ct_hash);
	emit messagePrepared(V1Parser().gen_HaveChunk(message));

	qDebug() << "==> HAVE_BLOCK:"
		<< "ct_hash=" << ct_hash_readable(ct_hash);
}

void MessageHandler::sendMetaRequest(const Meta::PathRevision& revision) {
	V1Parser::MetaRequest message;
	message.revision = revision;
	emit messagePrepared(V1Parser().gen_MetaRequest(message));

	qDebug() << "==> META_REQUEST:"
		<< "path_id=" << path_id_readable(revision.path_id_)
		<< "revision=" << revision.revision_;
}
void MessageHandler::sendMetaReply(const SignedMeta& smeta, const bitfield_type& bitfield) {
	V1Parser::MetaReply message;
	message.smeta = smeta;
	message.bitfield = bitfield;
	emit messagePrepared(V1Parser().gen_MetaReply(message));

	qDebug() << "==> META_REPLY:"
		<< "path_id=" << path_id_readable(smeta.meta().path_id())
		<< "revision=" << smeta.meta().revision()
		<< "bits=" << conv_bitarray(bitfield);
}
void MessageHandler::sendMetaCancel(const Meta::PathRevision& revision) {
	V1Parser::MetaCancel message;
	message.revision = revision;
	emit messagePrepared(V1Parser().gen_MetaCancel(message));

	qDebug() << "==> META_CANCEL:"
		<< "path_id=" << path_id_readable(revision.path_id_)
		<< "revision=" << revision.revision_;
}

void MessageHandler::sendBlockRequest(QByteArray ct_hash, uint32_t offset, uint32_t length) {
	V1Parser::BlockRequest message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	emit messagePrepared(V1Parser().gen_BlockRequest(message));

	qDebug() << "==> BLOCK_REQUEST:"
		<< "ct_hash=" << ct_hash_readable(ct_hash)
		<< "offset=" << offset
		<< "length=" << length;
}
void MessageHandler::sendBlockReply(QByteArray ct_hash, uint32_t offset, QByteArray block) {
	V1Parser::BlockReply message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.content = block;
	emit messagePrepared(V1Parser().gen_BlockReply(message));

	qDebug() << "==> BLOCK_REPLY:"
		<< "ct_hash=" << ct_hash_readable(ct_hash)
		<< "offset=" << offset;
}
void MessageHandler::sendBlockCancel(QByteArray ct_hash, uint32_t offset, uint32_t length) {
	V1Parser::BlockCancel message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	emit messagePrepared(V1Parser().gen_BlockCancel(message));
	qDebug() << "==> BLOCK_CANCEL:"
		<< "ct_hash=" << ct_hash_readable(ct_hash)
		<< "offset=" << offset
		<< "length=" << length;
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
	auto message_struct = V1Parser().parse_HaveMeta(message_raw);
	qDebug() << "<== HAVE_META:"
		<< "path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< "revision=" << message_struct.revision.revision_
		<< "bits=" << conv_bitarray(message_struct.bitfield);

	emit rcvdHaveMeta(message_struct.revision, message_struct.bitfield);
}
void MessageHandler::handleHaveChunk(QByteArray message_raw) {
	auto message_struct = V1Parser().parse_HaveChunk(message_raw);
	qDebug() << "<== HAVE_BLOCK:"
		<< "ct_hash=" << ct_hash_readable(message_struct.ct_hash);
	emit rcvdHaveChunk(conv_bytearray(message_struct.ct_hash));
}

void MessageHandler::handleMetaRequest(QByteArray message_raw) {
	auto message_struct = V1Parser().parse_MetaRequest(message_raw);
	qDebug() << "<== META_REQUEST:"
		<< "path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< "revision=" << message_struct.revision.revision_;

	emit rcvdMetaRequest(message_struct.revision);
}
void MessageHandler::handleMetaReply(QByteArray message_raw) {
	auto message_struct = V1Parser().parse_MetaReply(message_raw);
	qDebug() << "<== META_REPLY:"
		<< "path_id=" << path_id_readable(message_struct.smeta.meta().path_id())
		<< "revision=" << message_struct.smeta.meta().revision()
		<< "bits=" << conv_bitarray(message_struct.bitfield);

	emit rcvdMetaReply(message_struct.smeta, message_struct.bitfield);
}
void MessageHandler::handleMetaCancel(QByteArray message_raw) {
#   warning "Not implemented yet"
	auto message_struct = V1Parser().parse_MetaCancel(message_raw);
	qDebug() << "<== META_CANCEL:"
		<< "path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< "revision=" << message_struct.revision.revision_;

	emit rcvdMetaCancel(message_struct.revision);
}

void MessageHandler::handleBlockRequest(QByteArray message_raw) {
	auto message_struct = V1Parser().parse_BlockRequest(message_raw);
	qDebug() << "<== BLOCK_REQUEST:"
		<< "ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< "length=" << message_struct.length
		<< "offset=" << message_struct.offset;

	emit rcvdBlockRequest(message_struct.ct_hash, message_struct.offset, message_struct.length);
}
void MessageHandler::handleBlockReply(QByteArray message_raw) {
	auto message_struct = V1Parser().parse_BlockReply(message_raw);
	qDebug() << "<== BLOCK_REPLY:"
		<< "ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< "offset=" << message_struct.offset;

	emit rcvdBlockReply(message_struct.ct_hash, message_struct.offset, message_struct.content);
}
void MessageHandler::handleBlockCancel(QByteArray message_raw) {
#   warning "Not implemented yet"
	auto message_struct = V1Parser().parse_BlockCancel(message_raw);
	qDebug() << "<== BLOCK_CANCEL:"
		<< "ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< "length=" << message_struct.length
		<< "offset=" << message_struct.offset;

	emit rcvdBlockCancel(message_struct.ct_hash, message_struct.offset, message_struct.length);
}

} /* namespace librevault */
