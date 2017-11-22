/* Copyright (C) 2015 Alexander Shishenko <alex@shishenko.com>
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
#include "ChunkInfo.h"
#include "ChunkInfo_p.h"
#include "Secret.h"
#include "crypto/AES_CBC.h"
#include <QCryptographicHash>
#include <QDebug>

namespace librevault {

ChunkInfo::ChunkInfo() { d = new ChunkInfoPrivate; }
ChunkInfo::ChunkInfo(const ChunkInfo& r) = default;
ChunkInfo::ChunkInfo(ChunkInfo&& r) noexcept = default;
ChunkInfo::~ChunkInfo() = default;
ChunkInfo& ChunkInfo::operator=(const ChunkInfo& r) = default;
ChunkInfo& ChunkInfo::operator=(ChunkInfo&& r) noexcept = default;

QByteArray ChunkInfo::ctHash() const { return QByteArray::fromStdString(d->proto.ct_hash()); }
void ChunkInfo::ctHash(const QByteArray& ct_hash) { d->proto.set_ct_hash(ct_hash.toStdString()); }

quint64 ChunkInfo::size() const { return d->proto.size(); }
void ChunkInfo::size(quint64 size) { d->proto.set_size(size); }

QByteArray ChunkInfo::iv() const { return QByteArray::fromStdString(d->proto.iv()); }
void ChunkInfo::iv(const QByteArray& iv) { d->proto.set_iv(iv.toStdString()); }

QByteArray ChunkInfo::ptKeyedHash() const {
  return QByteArray::fromStdString(d->proto.pt_keyed_hash());
}
void ChunkInfo::ptKeyedHash(const QByteArray& pt_keyed_hash) {
  d->proto.set_pt_keyed_hash(pt_keyed_hash.toStdString());
}

QByteArray ChunkInfo::encrypt(const QByteArray& chunk, const Secret& secret, const QByteArray& iv) {
  return encryptAesCbc(chunk, secret.encryptionKey(), iv);
}

QByteArray ChunkInfo::decrypt(const QByteArray& chunk, const Secret& secret, const QByteArray& iv) {
  return decryptAesCbc(chunk, secret.encryptionKey(), iv);
}

QByteArray ChunkInfo::computeHash(const QByteArray& chunk) {
  QCryptographicHash hasher(QCryptographicHash::Sha3_256);
  hasher.addData(chunk);
  return hasher.result();
}

QByteArray ChunkInfo::computeKeyedHash(const QByteArray& chunk, const Secret& secret) {
  QCryptographicHash hasher(QCryptographicHash::Sha3_256);
  hasher.addData(secret.encryptionKey());
  hasher.addData(chunk);

  return hasher.result();
}

bool ChunkInfo::verifyChunk(const QByteArray& ct_hash, const QByteArray& chunk_pt) {
  return ct_hash == computeHash(chunk_pt);
}

QDebug operator<<(QDebug debug, const ChunkInfo& info) {
  QDebugStateSaver saver(debug);
  debug.nospace() << "ChunkInfo(ct_hash: " << info.ctHash().toHex() << ", size:" << info.size()
                  << ", iv:" << info.iv() << ", pt_keyed_hash:" << info.ptKeyedHash() << ")";
  return debug;
}

}  // namespace librevault
