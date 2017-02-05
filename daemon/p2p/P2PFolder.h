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
#include "folder/RemoteFolder.h"
#include "p2p/BandwidthCounter.h"
#include <QTimer>
#include <QWebSocket>

namespace librevault {

class FolderGroup;
class NodeKey;
class P2PProvider;

class P2PFolder : public RemoteFolder {
	Q_OBJECT
	friend class P2PProvider;
public:
	/* Errors */
	struct error : public std::runtime_error {
		error(const char* what) : std::runtime_error(what){}
	};
	struct protocol_error : public error {
		protocol_error() : error("Protocol error") {}
	};
	struct auth_error : public error {
		auth_error() : error("Remote node couldn't verify its authenticity") {}
	};

	P2PFolder(QUrl url, QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key);
	P2PFolder(QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key);
	~P2PFolder();

	/* Getters */
	QString displayName() const;
	QByteArray digest() const;
	QPair<QHostAddress, quint16> remote_endpoint() const;
	QString client_name() const {return client_name_;}
	QString user_agent() const {return user_agent_;}
	QJsonObject collect_state();

	/* RPC Actions */
	void send_message(const blob& message);

	// Handshake
	void sendHandshake();
	bool ready() const {return handshake_sent_ && handshake_received_;}

	/* Message senders */
	void choke();
	void unchoke();
	void interest();
	void uninterest();

	void post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield);
	void post_have_chunk(const blob& ct_hash);

	void request_meta(const Meta::PathRevision& revision);
	void post_meta(const SignedMeta& smeta, const bitfield_type& bitfield);
	void cancel_meta(const Meta::PathRevision& revision);

	void request_block(const blob& ct_hash, uint32_t offset, uint32_t size);
	void post_block(const blob& ct_hash, uint32_t offset, const blob& block);
	void cancel_block(const blob& ct_hash, uint32_t offset, uint32_t size);

private:
	enum Role {SERVER, CLIENT} role_;

	P2PFolder(QWebSocket* socket, FolderGroup* fgroup, P2PProvider* provider, NodeKey* node_key, Role role);

	P2PProvider* provider_;
	NodeKey* node_key_;
	QWebSocket* socket_;
	FolderGroup* fgroup_;

	/* Handshake */
	bool handshake_received_ = false;
	bool handshake_sent_ = false;

	BandwidthCounter counter_;

	/* These needed primarily for UI */
	QString client_name_;
	QString user_agent_;

	/* Ping/pong and timeout handlers */
	QTimer* ping_timer_;
	QTimer* timeout_timer_;

	/* Token generators */
	blob derive_token_digest(const Secret& secret, QByteArray digest);
	blob local_token();
	blob remote_token();

	void bump_timeout();

	std::chrono::milliseconds rtt_ = std::chrono::milliseconds(0);

	/* Message handlers */
	void handle_message(const QByteArray& message);

	void handle_Handshake(const blob& message_raw);

	void handle_Choke(const blob& message_raw);
	void handle_Unchoke(const blob& message_raw);
	void handle_Interested(const blob& message_raw);
	void handle_NotInterested(const blob& message_raw);

	void handle_HaveMeta(const blob& message_raw);
	void handle_HaveChunk(const blob& message_raw);

	void handle_MetaRequest(const blob& message_raw);
	void handle_MetaReply(const blob& message_raw);
	void handle_MetaCancel(const blob& message_raw);

	void handle_BlockRequest(const blob& message_raw);
	void handle_BlockReply(const blob& message_raw);
	void handle_BlockCancel(const blob& message_raw);

private slots:
	void handlePong(quint64 rtt);
	void handleWebSocketStateChanged(QAbstractSocket::SocketState state);
};

} /* namespace librevault */
