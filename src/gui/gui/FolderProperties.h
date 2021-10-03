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
#include <QDialog>
#include <QJsonObject>
#include <memory>

#include "Secret.h"
#include "ui_FolderProperties.h"

class Daemon;
class FolderModel;

class FolderProperties : public QDialog {
  Q_OBJECT

 public:
  FolderProperties(QByteArray folderid, Daemon* daemon, FolderModel* folder_model, QWidget* parent = 0);
  ~FolderProperties();

 protected:
  Ui::FolderProperties ui;
  void init_secrets();

 public slots:
  void refresh();

 private:
  Daemon* daemon_;
  FolderModel* folder_model_;
  QByteArray folderid_;
};
