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
#include <QFile>
#include <QObject>
#include <QSslCertificate>
#include <QSslKey>

#include "util/log.h"

Q_DECLARE_LOGGING_CATEGORY(log_nodekey)

namespace librevault {

class NodeKey : public QObject {
  Q_OBJECT
  LOG_SCOPE("NodeKey");

 public:
  NodeKey(QObject* parent);
  virtual ~NodeKey();

  QCryptographicHash::Algorithm digestAlgorithm() const { return QCryptographicHash::Sha256; }

  QByteArray digest() const;
  QSslKey privateKey() const { return private_key_; }
  QSslCertificate certificate() const { return certificate_; }

 private:
  void write_key();
  void gen_certificate();

  QFile cert_file_, private_key_file_;
  QSslKey private_key_;
  QSslCertificate certificate_;
};

}  // namespace librevault
