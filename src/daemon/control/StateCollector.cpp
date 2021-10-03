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
#include "StateCollector.h"

#include <QJsonArray>
#include <QLoggingCategory>

namespace librevault {

Q_LOGGING_CATEGORY(log_state, "state")

StateCollector::StateCollector(QObject* parent) : QObject(parent) {}
StateCollector::~StateCollector() {}

void StateCollector::global_state_set(QString key, QJsonValue value) {
  if (global_state_buffer[key] != value) {
    global_state_buffer[key] = value;
    qCDebug(log_state) << "Global state var" << key << "set to" << value;
    emit globalStateChanged(key, value);
  }
}

void StateCollector::folder_state_set(QByteArray folderid, QString key, QJsonValue value) {
  QJsonObject& folder_buffer = folder_state_buffers[folderid];
  if (folder_buffer[key] != value) {
    folder_buffer[key] = value;
    qCDebug(log_state) << "State of folder" << folderid.toHex() << "var" << key << "set to" << value;
    emit folderStateChanged(folderid, key, value);
  }
}

void StateCollector::folder_state_purge(QByteArray folderid) {
  if (folder_state_buffers.remove(folderid)) {
    qCDebug(log_state) << "Folder state" << folderid.toHex() << "purged";
  }
}

QJsonObject StateCollector::global_state() { return global_state_buffer; }

QJsonArray StateCollector::folder_state() {
  QJsonArray folder_state_all;
  for (const QJsonObject& state_o : folder_state_buffers.values()) {
    folder_state_all << state_o;
  }
  return folder_state_all;
}

QJsonObject StateCollector::folder_state(const QByteArray& folderid) { return folder_state_buffers.value(folderid); }

}  // namespace librevault
