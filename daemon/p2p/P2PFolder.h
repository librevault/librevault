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
#include "util/BandwidthCounter.h"
#include <QTimer>
#include <QWebSocket>
#include <chrono>
#include "blob.h"
#include <librevault/Meta.h>
#include <librevault/SignedMeta.h>
#include <librevault/util/conv_bitfield.h>
#include <QObject>

namespace librevault {

class FolderGroup;
class NodeKey;
class P2PProvider;

#define DECLARE_MESSAGE(message_name, fields...) \
public: \
	Q_SIGNAL void rcvd##message_name(fields); \
public: \
	void send##message_name(fields); \
private: \
	Q_SLOT void handle##message_name(const blob& message);

class P2PFolder : public QObject {
	Q_OBJECT
	friend class P2PProvider;

signals:
	void disconnected();

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

	P2PFolder(FolderGroup* fgroup, NodeKey* node_key, QObject* parent);
	~P2PFolder();

	void setConnectedSocket(QWebSocket* socket);
	void open(QUrl url);

	/* Getters */
	QString displayName() const;
	QByteArray digest() const;
	QPair<QHostAddress, quint16> endpoint() const;
	QString clientName() const {return client_name_;}
	QString userAgent() const {return user_agent_;}
	QJsonObject collectState();

	bool isValid() const {return handshakePassed();}

private:
	enum Role {SERVER, CLIENT} role_;

	QString log_tag() const {return displayName();}

	NodeKey* node_key_;
	QWebSocket* socket_ = nullptr;
	FolderGroup* fgroup_;

	BandwidthCounter counter_;

	/* These needed primarily for UI */
	QString client_name_;
	QString user_agent_;

	// Underlying socket management
	void resetUnderlyingSocket(QWebSocket* socket);

/* Token generators */
private:
	static blob derive_token_digest(const Secret& secret, QByteArray digest);
	blob local_token();
	blob remote_token();

/* Timeout */
private:
	QTimer* timeout_timer_;
	void startTimeout();
	void bumpTimeout();

/* Pinger */
private:
	QTimer* ping_timer_;
	std::chrono::milliseconds rtt_ = std::chrono::milliseconds(0);
	void startPinger();

private slots:
	void handlePong(quint64 rtt);

/* Choking status */
public:
	bool am_choking() const {return am_choking_;}
	bool am_interested() const {return am_interested_;}
	bool peer_choking() const {return peer_choking_;}
	bool peer_interested() const {return peer_interested_;}

private:
	bool am_choking_ = true;
	bool am_interested_ = false;
	bool peer_choking_ = true;
	bool peer_interested_ = false;

/* InterestGuard */
public:
	struct InterestGuard {
		InterestGuard(P2PFolder* remote);
		~InterestGuard();
	private:
		P2PFolder* remote_;
	};
	std::shared_ptr<InterestGuard> get_interest_guard();

private:
	std::weak_ptr<InterestGuard> interest_guard_;

/* State change handlers */
private slots:
	void handleDisconnected() {deleteLater();}
	void handleConnected();

/////////////// Message processing
DECLARE_MESSAGE(Handshake);

DECLARE_MESSAGE(Choke);
DECLARE_MESSAGE(Unchoke);
DECLARE_MESSAGE(Interested);
DECLARE_MESSAGE(NotInterested);

DECLARE_MESSAGE(HaveMeta, const Meta::PathRevision& revision, const bitfield_type& bitfield);
DECLARE_MESSAGE(HaveChunk, const blob& ct_hash);

DECLARE_MESSAGE(MetaRequest, const Meta::PathRevision& revision);
DECLARE_MESSAGE(MetaReply, const SignedMeta& smeta, const bitfield_type& bitfield);
DECLARE_MESSAGE(MetaCancel, const Meta::PathRevision& revision);

DECLARE_MESSAGE(BlockRequest, const blob& ct_hash, uint32_t offset, uint32_t size);
DECLARE_MESSAGE(BlockReply, const blob& ct_hash, uint32_t offset, const blob& block);
DECLARE_MESSAGE(BlockCancel, const blob& ct_hash, uint32_t offset, uint32_t size);

/* Handshake */
private:
	bool handshake_received_ = false, handshake_sent_ = false;
	bool handshakePassed() const {return handshake_sent_ && handshake_received_;}

signals:
	void handshakeSuccess();
	void handshakeFailed();

/* Message processing */
private slots:
	void sendMessage(const QByteArray& message);
	void sendMessage(const blob& message);
	void handleMessage(const QByteArray& message);
};

} /* namespace librevault */
