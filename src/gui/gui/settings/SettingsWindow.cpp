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
#include "SettingsWindow.h"
#ifndef Q_OS_MAC
#include "SettingsWindowPrivate.all.h"
#else
#include "SettingsWindowPrivate.mac.h"
#endif

SettingsWindow::SettingsWindow(QWidget* parent) : QDialog(parent) {
  ui_window_.setupUi(this);
  d = new SettingsWindowPrivate(this);

  //	connect(ui_bottom_.dialog_box, &QDialogButtonBox::accepted, this, &Settings::okayPressed);
  //	connect(ui_bottom_.dialog_box, &QDialogButtonBox::rejected, this, &Settings::cancelPressed);
}

SettingsWindow::~SettingsWindow() { delete d; }

void SettingsWindow::addPane(QWidget* pane) {
  pane->setParent(this);
  d->add_pane(pane);
}

void SettingsWindow::retranslateUi() {}
