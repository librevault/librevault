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
#include "UndefinedSession.h"

#include <QRegularExpression>
#include <QTcpSocket>
#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_undef_session, "webserver.undef")

static QRegularExpression header_regex(R"(^(\S+): (.+)\r\n)");

UndefinedSession::UndefinedSession(QTcpSocket* sock, QObject* parent)
    : QObject(parent), sock_(sock) {
  qCDebug(log_undef_session) << "Started session:" << this;

  sock->setParent(this);
  sock->startTransaction();
  connect(sock, &QIODevice::readyRead, this, &UndefinedSession::readHandler);
}

void UndefinedSession::readHandler() {
  last_header_buf_ += sock_->readLine(4096);

  if (last_header_buf_ == "\n" || last_header_buf_ == "\r\n") {
    sock_->rollbackTransaction();
    return haveHttp(sock_);
  }

  auto match = header_regex.match(QString::fromLatin1(last_header_buf_));
  if (match.hasMatch()) {
    QString name = match.captured(1);
    QString value = match.captured(2);

    qCDebug(log_undef_session) << "Got header:" << name << ":" << value;

    if (name.compare("Upgrade", Qt::CaseInsensitive) == 1 &&
        value.compare("WebSocket", Qt::CaseInsensitive) == 1) {
      sock_->rollbackTransaction();
      return haveWebSocket(sock_);
    }
  }

  if(last_header_buf_.right(1) == "\n"){ last_header_buf_.clear(); readHandler();}
}

}  // namespace librevault
