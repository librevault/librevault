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
#include <QStatusBar>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

class Daemon;
class StatusBar : public QObject {
  Q_OBJECT

 public:
  StatusBar(QStatusBar* bar, Daemon* daemon);
  ~StatusBar();

 public slots:
  void refresh();

 private:
  QStatusBar* bar_;
  Daemon* daemon_;

  QWidget* container_;
  QHBoxLayout* container_layout_;
  QLabel* dht_label_;
  QLabel* down_label_;
  QLabel* up_label_;

  QFrame* create_separator() const;

 private slots:
  void refreshBandwidth(float up_bandwidth, float down_bandwidth, double up_bytes, double down_bytes);

  void showDHTMenu(const QPoint& pos);
};
