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
#include "MulticastGroup.h"
#include "MulticastProvider.h"
#include <QLoggingCategory>
#include <QJsonObject>
#include <QJsonDocument>

Q_DECLARE_LOGGING_CATEGORY(log_multicast)

namespace librevault {

MulticastGroup::MulticastGroup(MulticastProvider* provider, QByteArray id) :
	provider_(provider), id_(id) {
	timer_ = new QTimer(this);
	timer_->setInterval(30*1000);

	// Connecting signals
	connect(timer_, &QTimer::timeout, this, &MulticastGroup::sendMulticasts);
	connect(provider_, &MulticastProvider::discovered, this, [=](QByteArray id, QHostAddress addr, quint16 port){
		if(id == id_) emit discovered(addr, port);
	});
}

void MulticastGroup::setEnabled(bool enabled) {
	if(!timer_->isActive() && enabled) {
		sendMulticasts();
		timer_->start();
	}else if(timer_->isActive() && !enabled)
		timer_->stop();
}

void MulticastGroup::setInterval(std::chrono::seconds interval) {
	timer_->setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(interval).count());
}

QByteArray MulticastGroup::getMessage() {
	QJsonObject message;
	message["port"] = provider_->getAnnouncePort();
	message["id"] = QString::fromLatin1(id_.toBase64());

	return QJsonDocument(message).toJson(QJsonDocument::Compact);
}

void MulticastGroup::sendMulticast(QUdpSocket* socket, QHostAddress addr, quint16 port) {
	if(! provider_->isEnabled()) return;
	if(socket->writeDatagram(getMessage(), addr, port))
		qCDebug(log_multicast) << "===> Multicast message sent to:" << addr << ":" << port;
	else
		qCDebug(log_multicast) << "=X=> Multicast message not sent to:" << addr << ":" << port << "E:" << socket->errorString();
}

void MulticastGroup::sendMulticasts() {
	sendMulticast(provider_->getSocket4(), provider_->getAddress4(), provider_->getMulticastPort4());
	sendMulticast(provider_->getSocket6(), provider_->getAddress6(), provider_->getMulticastPort6());
}

} /* namespace librevault */
