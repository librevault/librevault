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
#include "gui/settings/SettingsWindow.h"
#include "startup/StartupInterface.h"
#include "ui_Settings_General.h"
#include "ui_Settings_Network.h"

class Daemon;
class Updater;
class Translator;

class Settings : public SettingsWindow {
  Q_OBJECT

 public:
  explicit Settings(Daemon* daemon, Updater* updater, Translator* translator, QWidget* parent = 0);
  ~Settings();

 signals:
  void localeChanged(QString code);

 public slots:
  void retranslateUi();

 protected:
  Ui::Settings_General ui_pane_general_;
  Ui::Settings_Network ui_pane_network_;
  QWidget* pane_general_;
  QWidget* pane_network_;

  void init_ui();
  void reset_ui_states();
  void process_ui_states();

  Daemon* daemon_;
  Updater* updater_;
  Translator* translator_;
  StartupInterface* startup_interface_;

 private:
  void showEvent(QShowEvent* e) override;

 private slots:
  void okayPressed();
  void cancelPressed();
};
