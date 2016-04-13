/* Copyright (C) 2015-2016 Alexander Shishenko <GamePad64@gmail.com>
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
#include <QToolButton>
#include <QJsonObject>
#include <QUrl>
#include <gui/pager/Pager.h>
#include "pch.h"
#include "StartupInterface.h"

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
