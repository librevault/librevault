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
#include "parse_url.h"

#include <QtCore/QString>
#include <QtNetwork/QHostAddress>
#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/lexical_cast.hpp>

namespace librevault {

url::url(std::string url_str) {
  boost::algorithm::trim(url_str);
  std::string::const_iterator it_scheme_begin, it_scheme_end, it_userinfo_begin, it_userinfo_end, it_host_begin,
      it_host_end, it_port_begin, it_port_end, it_authority_begin, it_authority_end;

  // Scheme section
  it_scheme_begin = url_str.cbegin();
  auto schema_end_pos = url_str.find("://", 0);
  if (schema_end_pos != std::string::npos)
    it_scheme_end = url_str.cbegin() + schema_end_pos;
  else
    it_scheme_end = it_scheme_begin;
  scheme.assign(it_scheme_begin, it_scheme_end);

  // Authority section
  it_authority_begin = scheme.empty() ? it_scheme_end : it_scheme_end + 3;
  it_authority_end = std::find(it_authority_begin, url_str.cend(), '/');

  // Authority->Userinfo section
  it_userinfo_begin = it_authority_begin;
  it_userinfo_end = std::find(it_userinfo_begin, it_authority_end, '@');
  if (it_userinfo_end != it_authority_end) {
    it_host_begin = it_userinfo_end + 1;
  } else {
    it_userinfo_end = it_userinfo_begin;
    it_host_begin = it_userinfo_end;
  }
  userinfo.assign(it_userinfo_end, it_userinfo_end);

  // Authority->Host section
  if (*it_host_begin == '[') {  // We have IPv6 address.
    it_host_begin++;
    it_host_end = std::find(it_host_begin, it_authority_end, ']');
    is_ipv6 = true;
  } else {
    it_host_end = std::find(it_host_begin, it_authority_end, ':');
  }
  host = QString::fromStdString(std::string(it_host_begin, it_host_end));

  // Authority->Port section
  it_port_begin = std::find(it_host_end, it_authority_end, ':');
  it_port_end = it_authority_end;
  if (it_port_begin != it_port_end) {
    ++it_port_begin;
    port = boost::lexical_cast<uint16_t>(&*it_port_begin, std::distance(it_port_begin, it_port_end));
  }

  query = QString::fromStdString(std::string(it_authority_end, url_str.cend()));
}

url::url(const QString& url_str) : url(url_str.toStdString()) {}

url::operator QString() const {
  QString result;
  if (!scheme.empty()) result += QString::fromStdString(scheme) + "://";
  result += QString::fromStdString(userinfo);

  QHostAddress addr(host);
  if (is_ipv6) {
    result += QStringLiteral("[%1]").arg(addr.toString());
  } else
    result += host;
  if (port != 0) {
    result += ":";
    result += QString::number(port);
  }
  result += query;
  return result;
}

}  // namespace librevault
