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
#pragma once
#include <QAbstractSocket>
#include <QLoggingCategory>
#include <memory>
#include <set>

namespace librevault {

Q_DECLARE_LOGGING_CATEGORY(log_portmapping)

class NATPMPService;
class UPnPService;

struct MappingRequest {
  QAbstractSocket::SocketType protocol;
  quint16 orig_port = 0;
  quint16 mapped_port = 0;
  QString description;
};

class MappedPort : public QObject {
  Q_OBJECT

 public:
  MappedPort(QObject* parent) : QObject(parent) {}

  Q_SIGNAL void portMapped(quint16 mapped);
  virtual bool isMapped() const { return mapped_; }

 protected:
  bool mapped_ = false;
};

class MappingService : public QObject {
  Q_OBJECT

 public:
  MappingService(QObject* parent) : QObject(parent) {}
  ~MappingService();

  virtual MappedPort* map(const MappingRequest& mapping) = 0;

 protected:
  std::set<std::unique_ptr<MappedPort>> mappings_;
};

}  // namespace librevault
