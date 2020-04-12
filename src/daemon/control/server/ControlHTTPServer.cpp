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
#include "ControlHTTPServer.h"

#include <QJsonArray>

#include "Version.h"
#include "control/Config.h"
#include "control/StateCollector.h"

namespace librevault {

#define ADD_HANDLER(REGEX, HANDLER)                 \
  handlers_ << qMakePair(QRegularExpression(REGEX), \
                         [this](pconn conn, QRegularExpressionMatch match) { HANDLER(conn, match); })

ControlHTTPServer::ControlHTTPServer(ControlServer& cs, ControlServer::server& server, StateCollector& state_collector_)
    : cs_(cs), server_(server), state_collector_(state_collector_) {
  // config
  ADD_HANDLER(R"(^\/v1\/globals(?:\/(\w+?))?\/?$)", handle_globals_config);
  ADD_HANDLER(R"(^\/v1\/folders\/?$)", handle_folders_config_all);
  ADD_HANDLER(R"(^\/v1\/folders\/(?!state)(\w+?)\/?$)", handle_folders_config_one);

  // state
  ADD_HANDLER(R"(^\/v1\/state\/?$)", handle_globals_state);
  ADD_HANDLER(R"(^\/v1\/folders\/state\/?$)", handle_folders_state_all);
  ADD_HANDLER(R"(^\/v1\/folders\/(?!state)(\w+?)\/state\/?$)", handle_folders_state_one);

  // daemon
  ADD_HANDLER(R"(^\/v1\/version\/?$)", handle_version);
  ADD_HANDLER(R"(^\/v1\/restart\/?$)", handle_restart);
  ADD_HANDLER(R"(^\/v1\/shutdown\/?$)", handle_shutdown);
}

ControlHTTPServer::~ControlHTTPServer() {}

void ControlHTTPServer::on_http(websocketpp::connection_hdl hdl) {
  LOGFUNC();

  auto conn = server_.get_con_from_hdl(hdl);

  // CORS
  if (!cs_.check_origin(QString::fromStdString(conn->get_request_header("Origin")))) {
    conn->set_status(websocketpp::http::status_code::forbidden);
    conn->set_body(make_error_body("BAD_ORIGIN", "Origin not allowed"));
    return;
  }

  // URI handlers
  try {
    QString uri = QString::fromStdString(conn->get_request().get_uri());
    QRegularExpressionMatch match;
    for (const auto& handler : handlers_) {
      match = handler.first.match(uri);
      if (match.hasMatch()) {
        handler.second(conn, match);
        break;
      }
    }
    if (!match.hasMatch()) {
      conn->set_status(websocketpp::http::status_code::not_implemented);
      conn->set_body(make_error_body("", "Handler is not implemented"));
    }
  } catch (std::exception& e) {
    conn->set_status(websocketpp::http::status_code::internal_server_error);
    conn->set_body(make_error_body("", e.what()));
  }
}

void ControlHTTPServer::handle_version(pconn conn, QRegularExpressionMatch match) {
  QJsonObject o;
  o["version"] = Version::current().versionString();

  sendJson(QJsonDocument(o), http_code::ok, conn);
}

void ControlHTTPServer::handle_restart(pconn conn, QRegularExpressionMatch match) {
  conn->set_status(websocketpp::http::status_code::ok);
  emit cs_.restart();
}

void ControlHTTPServer::handle_shutdown(pconn conn, QRegularExpressionMatch match) {
  conn->set_status(websocketpp::http::status_code::ok);
  emit cs_.shutdown();
}

void ControlHTTPServer::handle_globals_config(pconn conn, QRegularExpressionMatch match) {
  if (conn->get_request().get_method() == "GET" && match.captured(1).isNull()) {
    sendJson(Config::get()->exportGlobals(), http_code::ok, conn);
  } else if (conn->get_request().get_method() == "PUT" && match.captured(1).isNull()) {
    conn->set_status(websocketpp::http::status_code::ok);

    QJsonDocument new_config = QJsonDocument::fromJson(QByteArray::fromStdString(conn->get_request_body()));

    Config::get()->importGlobals(new_config);
  } else if (conn->get_request().get_method() == "GET" && !match.captured(1).isNull()) {
    QJsonObject o;
    o["key"] = match.captured(1);
    o["value"] = QJsonValue::fromVariant(Config::get()->getGlobal(match.captured(1)));

    sendJson(QJsonDocument(o), http_code::ok, conn);
  } else if (conn->get_request().get_method() == "PUT" && !match.captured(1).isNull()) {
    conn->set_status(websocketpp::http::status_code::ok);

    QJsonObject o = QJsonDocument::fromJson(QByteArray::fromStdString(conn->get_request_body())).object();

    Config::get()->setGlobal(match.captured(1), o["value"].toVariant());
  } else if (conn->get_request().get_method() == "DELETE" && !match.captured(1).isNull()) {
    conn->set_status(websocketpp::http::status_code::ok);
    Config::get()->removeGlobal(match.captured(1));
  }
}

void ControlHTTPServer::handle_folders_config_all(pconn conn, QRegularExpressionMatch match) {
  if (conn->get_request().get_method() == "GET") sendJson(Config::get()->exportFolders(), http_code::ok, conn);
}

void ControlHTTPServer::handle_folders_config_one(pconn conn, QRegularExpressionMatch match) {
  QByteArray folderid = QByteArray::fromHex(match.captured(1).toLatin1());
  if (conn->get_request().get_method() == "GET") {
    sendJson(QJsonDocument(QJsonObject::fromVariantMap(Config::get()->getFolder(folderid))), http_code::ok, conn);
  } else if (conn->get_request().get_method() == "PUT") {
    conn->set_status(websocketpp::http::status_code::ok);
    QJsonObject new_value = QJsonDocument::fromJson(QByteArray::fromStdString(conn->get_request_body())).object();

    Config::get()->addFolder(new_value.toVariantMap());
  } else if (conn->get_request().get_method() == "DELETE") {
    conn->set_status(websocketpp::http::status_code::ok);
    Config::get()->removeFolder(folderid);
  }
}

void ControlHTTPServer::handle_globals_state(pconn conn, QRegularExpressionMatch match) {
  sendJson(QJsonDocument(state_collector_.global_state()), http_code::ok, conn);
}

void ControlHTTPServer::handle_folders_state_all(pconn conn, QRegularExpressionMatch match) {
  sendJson(QJsonDocument(state_collector_.folder_state()), http_code::ok, conn);
}

void ControlHTTPServer::handle_folders_state_one(pconn conn, QRegularExpressionMatch match) {
  QByteArray folderid = QByteArray::fromHex(match.captured(1).toLatin1());

  sendJson(QJsonDocument(state_collector_.folder_state(folderid)), http_code::ok, conn);
}

std::string ControlHTTPServer::make_error_body(const std::string& code, const std::string& description) {
  QJsonObject error_json;
  error_json["error_code"] = code.empty() ? "UNKNOWN" : QString::fromStdString(code);
  error_json["description"] = QString::fromStdString(description);
  return QJsonDocument(error_json).toJson().toStdString();
}

void ControlHTTPServer::sendJson(QJsonDocument json, http_code code, pconn conn) {
  conn->set_status(code);
  conn->append_header("Content-Type", "text/x-json");
  conn->set_body(json.toJson(QJsonDocument::Compact).toStdString());
}

}  // namespace librevault
