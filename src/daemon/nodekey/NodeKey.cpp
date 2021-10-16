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
#include "NodeKey.h"

#include <QDir>
#include <librevault_util/src/nodekey.rs.h>

#include "control/Paths.h"
#include "util/log.h"

Q_LOGGING_CATEGORY(log_nodekey, "nodekey")

namespace librevault {

NodeKey::NodeKey(QObject* parent) : QObject(parent) {
  SCOPELOG(log_nodekey);

  QFile cert_file_(QDir::fromNativeSeparators(Paths::get()->cert_path));
  QFile private_key_file_(QDir::fromNativeSeparators(Paths::get()->key_path));

  nodekey_write_new(rust::Str(private_key_file_.fileName().toStdString()));
  nodekey_write_new_cert(rust::Str(private_key_file_.fileName().toStdString()), rust::Str(cert_file_.fileName().toStdString()));

  private_key_file_.open(QIODevice::ReadOnly);
  cert_file_.open(QIODevice::ReadOnly);

  private_key_ = QSslKey(&private_key_file_, QSsl::Ec);
  certificate_ = QSslCertificate(&cert_file_);

  qCInfo(log_nodekey) << "PeerID:" << digest().toHex();
}

NodeKey::~NodeKey() { SCOPELOG(log_nodekey); }

QByteArray NodeKey::digest() const { return certificate().digest(digestAlgorithm()); }

}  // namespace librevault
