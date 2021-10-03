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
 */
#pragma once
#include <QString>
#include <cstdint>
#include <string>

namespace librevault {

struct url {
  url() {}
  url(std::string str);
  url(const QString& str);
  std::string scheme;
  std::string userinfo;
  QString host;
  uint16_t port = 0;
  QString query;

  bool is_ipv6 = false;

  operator QString() const;
  bool operator==(const url& u) const {
    return (scheme == u.scheme) && (userinfo == u.userinfo) && (host == u.host) && (port == u.port) &&
           (query == u.query);
  }
  [[nodiscard]] bool empty() const { return *this == url(); }  // Not optimal, but simple
};

}  // namespace librevault
