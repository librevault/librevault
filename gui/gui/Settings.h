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

#include <QDialog>
#include <QToolButton>
#include <QJsonObject>
#include <QUrl>
#include <gui/pager/Pager.h>
#include "pch.h"
#include "startup/StartupInterface.h"

namespace Ui {
class Settings;
}

class Settings : public QDialog {
Q_OBJECT

public:
	explicit Settings(QWidget* parent = 0);
	~Settings();

	enum Page {
		PAGE_GENERAL = 0,
		PAGE_NETWORK = 1//,
		//PAGE_ACCOUNT = 2,
		//PAGE_ADVANCED = 3
	};

signals:
	void newConfigIssued(QJsonObject config_json);

public slots:
	void selectPage(int page);
	void retranslateUi();
	void handleControlJson(QJsonObject control_json);

protected:
	std::unique_ptr<Ui::Settings> ui;
	QJsonObject control_json_dynamic, control_json_static;

	void init_ui();
	void reset_ui_states();
	void process_ui_states();

	// Selector
	void init_selector();
	QString page_name(Page page);

	// Overrides
	void showEvent(QShowEvent* e) override;

private slots:
	void okayPressed();
	void applyPressed();
	void cancelPressed();

protected:
	Pager* pager;
	std::unique_ptr<StartupInterface> startup_interface;
	QUrl p2p_listen;
};
