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
#include "Session.h"

#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_session, "webserver.session");

Session::Session(const QUuid& sessid, QTcpSocket* sock, const HttpRequest& request, QObject* parent)
    : QObject(parent), sessid_(sessid), sock_(sock), request_(request) {
  sock_->setParent(this);
  timer_ = new QTimer(this);

  qCDebug(log_session) << "Started session:" << sessionId() << "this:" << this;

  connect(timer_, &QTimer::timeout, this, [=, this] { emit timeout(sessionId()); });
}

Session::~Session() {
  qCDebug(log_session) << "Stopped session:" << sessionId() << "this:" << this;
}

}  // namespace librevault
