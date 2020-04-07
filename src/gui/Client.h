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
#include <QApplication>
#include <QTranslator>
#include <QtSingleApplication>

#include "updater/Updater.h"

class MainWindow;
class Settings;
class TrayIcon;
class FolderModel;
class Daemon;
class RemoteConfig;
class Translator;

class Client : public QtSingleApplication {
  Q_OBJECT

 public:
  Client(int& argc, char** argv, int appflags = ApplicationFlags);
  ~Client();

  bool event(QEvent* event);

 private:
  Translator* translator_;
  Daemon* daemon_;
  FolderModel* folder_model_;

  Updater* updater_;

  // GUI
  MainWindow* main_window_;

  QString pending_link_;

  bool allow_multiple;

 private slots:
  void started();
  void singleMessageReceived(const QString& message);
};
