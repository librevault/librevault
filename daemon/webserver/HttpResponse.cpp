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
#include "HttpResponse.h"
#include <QTextStream>

namespace librevault {

static QHash<quint16, QString> code_map = {
    {200, "OK"}, {404, "Not Found"}, {500, "Internal Server Error"}};

void HttpResponse::setCode(quint16 code, const QString& comment) {
  code_ = code;
  code_comment_ = comment.isEmpty() ? code_map.value(code_, comment) : comment;
}

void HttpResponse::setDate(QDateTime datetime) {
  datetime = datetime.toUTC();
  QString date_string = QLocale::c().toString(datetime, "ddd, dd MMM yyyy hh:mm:ss") + " GMT";
  headers_["Date"] = QStringList{date_string};
}

QByteArray HttpResponse::makeResponse() const {
  QByteArray result;

  HeaderMap auto_headers;
  for (const auto& key : headers_.keys()) auto_headers[fixupHeaderName(key)] = headers_[key];
  if (auto_length) auto_headers["Content-Length"] = QStringList{QString::number(data_.size())};

  result += writeHeaders(auto_headers);
  result += data_;

  return result;
}

QString HttpResponse::fixupHeaderName(const QString& header) {
  QStringList words = header.split('-');
  for (auto& word : words) {
    word = word.toLower();
    word[0] = word[0].toUpper();
  }
  return words.join('-');
}

QByteArray HttpResponse::writeHeaders(const HeaderMap& header_map) const {
  QByteArray result;
  QTextStream header_stream(&result, QIODevice::WriteOnly);

  header_stream << QStringLiteral("HTTP/%1 %2 %3").arg(http_, QString::number(code_), code_comment_)
                << "\r\n";
  for (const auto& header_name : header_map.keys())
    for (const auto& header_val : header_map[header_name])
      header_stream << QStringLiteral("%1: %2").arg(header_name, header_val) << "\r\n";
  header_stream << "\r\n";

  return result;
}

}  // namespace librevault
