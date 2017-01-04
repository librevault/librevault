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
#include "control/Config.h"
#include "control/Paths.h"
#include "folder/FolderGroup.h"
#include "nat/PortMappingService.h"
#include "nodekey/NodeKey.h"
#include "util/log.h"

namespace librevault {

P2PProvider::P2PProvider(NodeKey& node_key, PortMappingService& port_mapping, FolderService& folder_service, QObject* parent) : QObject(parent),
	node_key_(node_key) {
	server_ = new QWebSocketServer(Version().user_agent(), QWebSocketServer::SecureMode, this);
	server_->setSslConfiguration(getSslConfiguration());

	connect(server_, &QWebSocketServer::newConnection, this, &P2PProvider::handleConnection);

	server_->listen(QHostAddress::Any, Config::get()->global_get("p2p_listen").asUInt());
}

P2PProvider::~P2PProvider() {}

QSslConfiguration P2PProvider::getSslConfiguration() {
	QSslConfiguration config;
	config.setPeerVerifyMode(QSslSocket::VerifyPeer);
	config.setPrivateKey(node_key_.privateKey());
	config.setLocalCertificate(node_key_.certificate());
	return config;
}

void P2PProvider::mark_loopback(const tcp_endpoint& endpoint) {
	loopback_blacklist_.emplace(endpoint);
	LOGN("Marked " << endpoint << " as loopback");
}

bool P2PProvider::is_loopback(const tcp_endpoint& endpoint) {
	return loopback_blacklist_.find(endpoint) != loopback_blacklist_.end();
}

bool P2PProvider::is_loopback(const QByteArray& digest) {
	return node_key_.digest() == digest;
}

/* Here are where new QWebSocket created */
void P2PProvider::handleConnection() {
	while(server_->hasPendingConnections()) {
		QWebSocket* socket = server_->nextPendingConnection();
		qDebug() << socket->requestUrl();
	}
}

P2PFolder* P2PProvider::startConnection(DiscoveryResult result, FolderGroup* fgroup) {
	QUrl ws_url = result.url;
	ws_url.setScheme("wss");
	ws_url.setPath(QString("/")+fgroup->folderid().toHex());
	if(ws_url.isValid()) {
		ws_url.setHost(result.address.toString());
		ws_url.setPort(result.port);
	}

	qDebug() << "New connection: " << ws_url;

	QWebSocket* socket = new QWebSocket(Version().user_agent(), QWebSocketProtocol::VersionLatest, this);
	P2PFolder* folder;
	return folder;
}

} /* namespace librevault */
