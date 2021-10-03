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

#include "ui_SettingsWindow.h"
#include "ui_SettingsWindow_bottom.h"

class SettingsWindowPrivate;

class SettingsWindow : public QDialog {
  Q_OBJECT

  friend class SettingsWindowPrivate;

 public:
  explicit SettingsWindow(QWidget* parent = 0);
  ~SettingsWindow();

  /**
   * Adds a new preferences pane to Settings window.
   * An icon will be taken from pane.windowIcon
   * The title will be taken from pane.windowTitle
   * This SettingsWindow object will take ownership of pane
   * @param pane
   * @return
   */
  void addPane(QWidget* pane);

 public slots:
  void retranslateUi();

 protected:
  Ui::SettingsWindow ui_window_;
  Ui::SettingsWindow_bottom ui_bottom_;

 protected:
  SettingsWindowPrivate* d;
};
