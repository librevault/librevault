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
#pragma once
#include "control/FolderParams.h"
#include "Secret.h"
#include "MetaInfo.h"
#include "SignedMeta.h"
#include <QBitArray>
#include <QObject>

namespace librevault {

#define DECLARE_MESSAGE(message_name, fields...) \
public: \
	Q_SIGNAL void rcvd##message_name(fields); \
public: \
	void send##message_name(fields); \
public: \
	Q_SLOT void handle##message_name(QByteArray message);

class MessageHandler : public QObject {
	Q_OBJECT

public:
	MessageHandler(QObject* parent = nullptr);

	Q_SIGNAL void messagePrepared(QByteArray msg);

private:
	// Messages
	DECLARE_MESSAGE(Choke);
	DECLARE_MESSAGE(Unchoke);
	DECLARE_MESSAGE(Interested);
	DECLARE_MESSAGE(NotInterested);

	DECLARE_MESSAGE(HaveMeta, const MetaInfo::PathRevision& revision, QBitArray bitfield);
	DECLARE_MESSAGE(HaveChunk, QByteArray ct_hash);

	DECLARE_MESSAGE(MetaRequest, const MetaInfo::PathRevision& revision);
	DECLARE_MESSAGE(MetaReply, const SignedMeta& smeta, QBitArray bitfield);
	DECLARE_MESSAGE(MetaCancel, const MetaInfo::PathRevision& revision);

	DECLARE_MESSAGE(BlockRequest, QByteArray ct_hash, uint32_t offset, uint32_t size);
	DECLARE_MESSAGE(BlockReply, QByteArray ct_hash, uint32_t offset, QByteArray block);
	DECLARE_MESSAGE(BlockCancel, QByteArray ct_hash, uint32_t offset, uint32_t size);
};

} /* namespace librevault */
