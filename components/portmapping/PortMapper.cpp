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
#include "PortMapper.h"
#include "NATPMPService.h"
#include "UPnPService.h"

namespace librevault {

Q_LOGGING_CATEGORY(log_portmapping, "portmapping")

PortMapper::PortMapper(QObject* parent) : QObject(parent) {
	natpmp_service_ = new NATPMPService(*this);
	upnp_service_ = new UPnPService(*this);

	auto port_callback = [this](QString id, quint16 port) {
		qCInfo(log_portmapping) << "Port mapped:" << mappings_[id].orig_port << "->" << port;
		mappings_[id].mapped_port = port;
	};
	connect(natpmp_service_, &NATPMPService::portMapped, port_callback);
	connect(upnp_service_, &UPnPService::portMapped, port_callback);
}

PortMapper::~PortMapper() {
	mappings_.clear();
}

void PortMapper::addPort(QString id, quint16 port, QAbstractSocket::SocketType protocol, QString description) {
	Mapping m;
	m.orig_port = port;
	m.protocol = protocol;
	m.description = description;
	mappings_[id] = m;

	natpmp_service_->addPort(id, m);
	upnp_service_->addPort(id, m);
}

void PortMapper::removePort(QString id) {
	upnp_service_->removePort(id);
	natpmp_service_->removePort(id);

	mappings_.remove(id);
}

quint16 PortMapper::getOriginalPort(QString id) {
	return mappings_.contains(id) ? mappings_[id].orig_port : 0;
}

quint16 PortMapper::getMappedPort(QString id) {
	return (mappings_.contains(id) && mappings_[id].mapped_port != 0) ? mappings_[id].mapped_port : getOriginalPort(id);
}

} /* namespace librevault */
