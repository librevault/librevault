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
#include "P2PProvider.h"
#include "P2PFolder.h"
#include "Version.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "folder/FolderService.h"
#include "PortMapper.h"
#include "nodekey/NodeKey.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_p2p, "p2p")

namespace librevault {

P2PProvider::P2PProvider(NodeKey* node_key,
                         PortMapper* port_mapping,
                         FolderService* folder_service,
                         QObject* parent) : QObject(parent),
	node_key_(node_key), port_mapping_(port_mapping), folder_service_(folder_service) {
	server_ = new QWebSocketServer(Version().version_string(), QWebSocketServer::SecureMode, this);
	server_->setSslConfiguration(getSslConfiguration());

	connect(server_, &QWebSocketServer::newConnection, this, &P2PProvider::handleConnection);
	connect(server_, &QWebSocketServer::peerVerifyError, this, &P2PProvider::handlePeerVerifyError);
	connect(server_, &QWebSocketServer::serverError, this, &P2PProvider::handleServerError);
	connect(server_, &QWebSocketServer::sslErrors, this, &P2PProvider::handleSslErrors);
	connect(server_, &QWebSocketServer::acceptError, this, &P2PProvider::handleAcceptError);

	if(server_->listen(QHostAddress::Any, Config::get()->getGlobal("p2p_listen").toUInt())) {
		qCInfo(log_p2p) << "Librevault is listening on port:" << server_->serverPort();
	}else{
		qCWarning(log_p2p) << "Librevault failed to bind on port:" << server_->serverPort() << "E:" << server_->errorString();
	}
	port_mapping_->addPort("main", server_->serverPort(), QAbstractSocket::TcpSocket, "Librevault");
}

P2PProvider::~P2PProvider() {
	port_mapping_->removePort("main");
}

QSslConfiguration P2PProvider::getSslConfiguration() const {
	QSslConfiguration ssl_config;
	ssl_config.setPeerVerifyMode(QSslSocket::QueryPeer);
	ssl_config.setPrivateKey(node_key_->privateKey());
	ssl_config.setLocalCertificate(node_key_->certificate());
	ssl_config.setProtocol(QSsl::TlsV1_2OrLater);
	return ssl_config;
}

bool P2PProvider::isLoopback(QByteArray digest) {
	return node_key_->digest() == digest;
}

QUrl P2PProvider::makeUrl(QHostAddress addr, quint16 port, QByteArray folderid) {
	QUrl url;
	url.setScheme("wss");
	url.setPath("/" + folderid.toHex());
	url.setHost(addr.toString());
	url.setPort(port);
}

/* Here are where new QWebSocket created */
void P2PProvider::handleConnection() {
	while(server_->hasPendingConnections()) {
		QWebSocket* socket = server_->nextPendingConnection();
		QUrl ws_url = socket->requestUrl();
		QByteArray folderid = QByteArray::fromHex(ws_url.path().mid(1).toUtf8());
		FolderGroup* fgroup = folder_service_->getGroup(folderid);

		qCDebug(log_p2p) << "New incoming connection:" << socket->requestUrl().toString();

		P2PFolder* folder = new P2PFolder(socket, fgroup, this, node_key_);
		Q_UNUSED(folder);
	}
}

void P2PProvider::handleDiscovered(QByteArray folderid, QHostAddress addr, quint16 port) {
	qCDebug(log_p2p) << "Discovery event about:" << addr << port;

	FolderGroup* fgroup = folder_service_->getGroup(folderid);
	if(!fgroup) {
		return; // Maybe, we have received a multicast not for us?
	}

	QUrl ws_url = makeUrl(addr, port, folderid);

	qCDebug(log_p2p) << "New connection:" << ws_url.toString();

	QWebSocket* socket = new QWebSocket(Version().user_agent(), QWebSocketProtocol::VersionLatest, this);
	P2PFolder* folder = new P2PFolder(ws_url, socket, fgroup, this, node_key_);
	Q_UNUSED(folder);
}

void P2PProvider::handlePeerVerifyError(const QSslError& error) {
	qCDebug(log_p2p) << "PeerVerifyError:" << error.errorString();
}

void P2PProvider::handleServerError(QWebSocketProtocol::CloseCode closeCode) {
	qCDebug(log_p2p) << "ServerError:" << server_->errorString();
}

void P2PProvider::handleSslErrors(const QList<QSslError>& errors) {
	qCDebug(log_p2p) << "SSL errors:" << errors;
}

void P2PProvider::handleAcceptError(QAbstractSocket::SocketError socketError) {
	qCDebug(log_p2p) << "Accept error:" << socketError;
}

} /* namespace librevault */
