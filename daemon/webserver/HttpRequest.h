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
#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include "util/exception.hpp"

namespace librevault {

class HttpRequest {
 public:
  DECLARE_EXCEPTION(ParseError, "HTTP request parsing failed");
  DECLARE_EXCEPTION_DETAIL(NoContentLength, ParseError,
      "Content-Length header not found, and chunked transfer is not supported");
  DECLARE_EXCEPTION_DETAIL(InvalidHeader, ParseError, "HTTP header is invalid");
  DECLARE_EXCEPTION_DETAIL(DataOverflow, ParseError, "Too much request data");

  using HeaderMap = QHash<QString, QStringList>;

  void addData(const QByteArray& data);
  bool atEnd() const { return complete_1st_line && complete_header && content_length_ == 0; };

  const QString& method() const {return method_;}
  const QString& path() const {return path_;}
  const QString& httpVersion() const {return http_;}
  const HeaderMap& headers() const { return headers_; }

 private:
  bool complete_1st_line = false;
  bool complete_header = false;
  int content_length_ = 0;
  QByteArray incomplete_header_buf;

  QString method_, path_, http_;
  QByteArray request_data_;

  HeaderMap headers_;

  bool parseLine(QByteArray line);
  void processHeaders();
};

}  // namespace librevault
