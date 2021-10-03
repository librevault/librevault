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
