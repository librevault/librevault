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
#include <QMacToolbar>
#include <QShowEvent>
#include <QVBoxLayout>

class SettingsWindow;
class SettingsWindowPrivate {
 public:
  SettingsWindowPrivate(SettingsWindow* window);

  void add_pane(QWidget* pane);

 private:
  // Window
  SettingsWindow* window_;
  QVBoxLayout* window_layout_;
  QVBoxLayout* pane_layout_;

  // Pager
  QMacToolBar* toolbar;
  QList<QMacToolBarItem*> buttons_;
  QList<QWidget*> panes_;

 private slots:
  void showPane(int pane);
};
