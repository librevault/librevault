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
#include "Secret.h"

namespace librevault {

class AesCbcData {
 public:
  QByteArray ct() const;
  QByteArray iv() const;

  void setEncrypted(const std::string& ct, const std::string& iv);

  bool check() const { return !ct_.isEmpty() && ct_.size() % 16 == 0 && iv_.size() == 16; };
  // Use this with extreme care. Can cause padding oracle attack, if misused. Meta is
  // (generally) signed and unmalleable
  void set_plain(const QByteArray& pt, const Secret& secret);
  QByteArray get_plain(const Secret& secret) const;  // Caching, maybe?
 private:
  QByteArray ct_, iv_;
};

}  // namespace librevault