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
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QObject>

namespace librevault {

class StateCollector : public QObject {
  Q_OBJECT
 public:
  StateCollector(QObject* parent);
  ~StateCollector();

  void global_state_set(QString key, QJsonValue value);
  void folder_state_set(QByteArray folderid, QString key, QJsonValue value);
  void folder_state_purge(QByteArray folderid);

  QJsonObject global_state();
  QJsonArray folder_state();
  QJsonObject folder_state(const QByteArray& folderid);

 signals:
  void globalStateChanged(QString key, QJsonValue value);
  void folderStateChanged(QByteArray folderid, QString key, QJsonValue value);

 private:
  QJsonObject global_state_buffer;
  QMap<QByteArray, QJsonObject> folder_state_buffers;
};

}  // namespace librevault
