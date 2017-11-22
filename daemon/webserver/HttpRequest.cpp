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
#include "HttpRequest.h"
#include <QBuffer>
#include <QDebug>
#include <QLoggingCategory>
#include <QRegularExpression>

namespace librevault {

Q_LOGGING_CATEGORY(log_parser, "webserver.http_parser")

static QRegularExpression http_1st_line(R"(^(\S+) (\S+) HTTP\/(\d\.\d)$)");
static QRegularExpression header_regex(R"(^(\S+): (.+)$)");

void HttpRequest::addData(const QByteArray& data) {
  QBuffer data_buf;
  data_buf.setData(data);
  data_buf.open(QIODevice::ReadOnly);

  while (!complete_header && !data_buf.atEnd()) {
    incomplete_header_buf += data_buf.readLine(4096);

    if (!incomplete_header_buf.endsWith("\r\n")) {
      qCDebug(log_parser) << "Got incomplete line: " << incomplete_header_buf;
      continue;
    }

    complete_header = parseLine(incomplete_header_buf);

    if (complete_header) processHeaders();

    incomplete_header_buf.clear();
  }

  if (complete_header && content_length_ > 0 && !data_buf.atEnd()) {
    QByteArray read_data = data_buf.readAll();
    request_data_ += read_data;
    content_length_ -= read_data.size();

    if (content_length_ == 0)
      qCDebug(log_parser) << "Got data:" << request_data_;
    else if (content_length_ < 0)
      throw DataOverflow();
  }
}

bool HttpRequest::parseLine(QByteArray line) {
  line = line.trimmed();

  if (!complete_1st_line) {
    auto match = http_1st_line.match(QString::fromLatin1(line));
    if (!match.hasMatch()) throw InvalidHeader();

    method_ = match.captured(1).toUpper();
    path_ = QByteArray::fromPercentEncoding(match.captured(2).toLatin1());
    http_ = match.captured(3);

    qCDebug(log_parser) << "Method:" << method_ << "Path:" << path_ << "Version" << http_;
    complete_1st_line = true;
  } else if (!line.isEmpty()) {
    auto match = header_regex.match(QString::fromLatin1(line));
    if (!match.hasMatch()) throw InvalidHeader();

    QString name = match.captured(1).toLower();
    QString value = match.captured(2);

    qCDebug(log_parser) << "Got header:" << name << ":" << value;
    headers_.insert(name, {value});
  }

  return line.isEmpty();
}

void HttpRequest::processHeaders() {
  if (method_ != "GET" && headers_.contains("content-length")) {
    if (!headers_.contains("content-length")) throw NoContentLength();

    content_length_ = headers_.value("content-length").first().toInt();
  }
}

}  // namespace librevault
