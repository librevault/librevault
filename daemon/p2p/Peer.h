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
#include "blob.h"
#include "TimeoutHandler.h"
#include <librevault/Meta.h>
#include <librevault/SignedMeta.h>
#include <librevault/util/conv_bitfield.h>
#include <QObject>

namespace librevault {

class FolderGroup;
class NodeKey;
class HandshakeHandler;
class PingHandler;
class MessageHandler;
class FolderParams;

class Peer : public QObject {
	Q_OBJECT

signals:
	void disconnected();
	void handshakeSuccess();
	void handshakeFailed(QString error);

public:
	Peer(const FolderParams& params, NodeKey* node_key, BandwidthCounter* bc_all, BandwidthCounter* bc_blocks, QObject* parent);
	~Peer();

	void setConnectedSocket(QWebSocket* socket);
	void open(QUrl url);

	static QUrl makeUrl(QPair<QHostAddress, quint16> endpoint, QByteArray folderid);

	/* Getters */
	QByteArray digest() const;
	QPair<QHostAddress, quint16> endpoint() const;
	QString endpointString() const;
	QString clientName() const;
	QString userAgent() const;
	QJsonObject collectState();

	bool isValid() const;

	MessageHandler* messageHandler() {return message_handler_;}

private:
	enum class Role {UNDEFINED = 0, SERVER = 1, CLIENT = 2} role_ = Role::UNDEFINED;

	QString log_tag() const {return endpointString();}

	NodeKey* node_key_;
	QWebSocket* socket_ = nullptr;

	BandwidthCounter bc_all_, bc_blocks_;

	PingHandler* ping_handler_;
	TimeoutHandler* timeout_handler_;
	HandshakeHandler* handshake_handler_;
	MessageHandler* message_handler_;

	// Underlying socket management
	void resetUnderlyingSocket(QWebSocket* socket);

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
		InterestGuard(Peer* remote);
		~InterestGuard();
	private:
		Peer* remote_;
	};
	std::shared_ptr<InterestGuard> get_interest_guard();

private:
	std::weak_ptr<InterestGuard> interest_guard_;

/* State change handlers */
private slots:
	void handleDisconnected() {disconnected();}
	void handleConnected();

/* Message processing */
private slots:
	void sendMessage(QByteArray message);
	void handleMessage(const QByteArray& message);
};

} /* namespace librevault */
