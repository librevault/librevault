/* Copyright (C) 2014-2017 Alexander Shishenko <alex@shishenko.com>
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
#include <QDataStream>
#include <boost/lexical_cast.hpp>

namespace librevault {
namespace protocol {

template <class To, class From>
To convert(const From& from) {
  return boost::lexical_cast<To>(from);
};

template <>
std::string convert(const QByteArray& from) {
  return from.toStdString();
};

template <>
QByteArray convert(const std::string& from) {
  return QByteArray::fromStdString(from);
};

template <>
std::string convert(const QString& from) {
  return from.toStdString();
};

template <>
QString convert(const std::string& from) {
  return QString::fromStdString(from);
};

template <>
std::string convert(const QBitArray& from) {
  QByteArray buffer;
  QDataStream stream(&buffer, QIODevice::Append);
  stream.setByteOrder(QDataStream::BigEndian);
  stream.setVersion(QDataStream::Qt_5_0);
  return buffer.toStdString();
};

template <>
QBitArray convert(const std::string& from) {
  QByteArray buffer_in = QByteArray::fromRawData(from.data(), from.size());
  QBitArray result;
  QDataStream stream(buffer_in);
  stream.setByteOrder(QDataStream::BigEndian);
  stream.setVersion(QDataStream::Qt_5_0);
  stream >> result;
  return result;
};

}  // namespace protocol
}  // namespace librevault
