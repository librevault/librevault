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

#include <QStatusBar>
#include <QJsonObject>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include "pch.h"

class StatusBar : public QObject {
Q_OBJECT

public:
	StatusBar(QStatusBar* bar);
	~StatusBar();

public slots:
	void handleControlJson(QJsonObject state_json);

	void refreshDHT(unsigned nodes);
	void refreshBandwidth(float up_bandwidth, float down_bandwidth, double up_bytes, double down_bytes);

protected:
	QStatusBar* bar_;

	QWidget* container_;
	QHBoxLayout* container_layout_;
	QLabel* dht_label_;
	QLabel* down_label_;
	QLabel* up_label_;

	QFrame* create_separator() const;
};
