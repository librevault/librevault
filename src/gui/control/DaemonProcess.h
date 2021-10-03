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
#include <QProcess>

class DaemonProcess : public QProcess {
  Q_OBJECT

 public:
  DaemonProcess(QObject* parent);
  ~DaemonProcess();

  void launch();

 signals:
  void daemonReady(const QUrl& control_url);
  void daemonFailed(QString reason);

 private slots:
  void handleError(QProcess::ProcessError error);
  void handleStandardOutput();

 protected:
  bool listening_already = false;
  QString get_executable_path() const;
};
