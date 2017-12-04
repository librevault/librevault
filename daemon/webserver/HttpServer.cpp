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
#include "HttpServer.h"

#include "HttpResponse.h"
#include "control/Config.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QLoggingCategory>
#include <QPair>
#include <QRegularExpression>
#include <QUrl>
#include <QTcpSocket>

namespace librevault {

Q_LOGGING_CATEGORY(log_http_session, "webserver.session.http")

HttpServer::HttpServer(QObject* parent) : QObject(parent) {
  registerHandler(R"(^\/v1\/config)", [](HandlerContext& ctx){
    ctx.response.setData(Config::get()->exportGlobals().toJson(QJsonDocument::Compact));
  });
}

void HttpServer::handleConnection(
    const QUuid& sessid, QTcpSocket* sock, const HttpRequest& request) {
  sock->setParent(this);
  qCDebug(log_http_session) << "Handling HTTP request from:" << sock->peerAddress()
                            << "sessid:" << sessid;

  HttpResponse response;
  response.headers()["Connection"] = QStringList{"close"};
  response.headers()["Content-Type"] = QStringList{"application/json"};
  response.setDate(QDateTime::currentDateTimeUtc());

  try {
    for (auto& handler : handlers_) {
      auto match = handler.first.match(request.path());
      if (!match.hasMatch()) continue;

      HandlerContext context{sessid, request, response, match};
      handler.second(context);
      break;
    }
  } catch (const std::exception& e) {
    QJsonObject obj;
    obj["error"] = e.what();

    response.setCode(500);
    response.setData(QJsonDocument(obj).toJson(QJsonDocument::Compact));
  }

  qCDebug(log_http_session) << response.makeResponse();
  sock->write(response.makeResponse());

  connect(sock, &QTcpSocket::disconnected, this, [=] { sock->deleteLater(); });
}

void HttpServer::registerHandler(const QString& regex, Handler handler) {
  handlers_.push_back({QRegularExpression(regex), handler});
}

}  // namespace librevault
