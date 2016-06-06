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
#include "Settings.h"
#include "ui_Settings.h"
#include <QCloseEvent>
#include <QDebug>
#include "icons/GUIIconProvider.h"
#include "appver.h"
#include "MainWindow.h"

Settings::Settings(QWidget* parent) :
		QDialog(parent),
		ui(std::make_unique<Ui::Settings>()) {
	startup_interface = std::make_unique<StartupInterface>();

	init_ui();
	connect(ui->dialog_box, &QDialogButtonBox::accepted, this, &Settings::okayPressed);
	connect(ui->dialog_box->button(QDialogButtonBox::StandardButton::Apply), &QPushButton::clicked, this, &Settings::applyPressed);
	connect(ui->dialog_box, &QDialogButtonBox::rejected, this, &Settings::cancelPressed);
}

Settings::~Settings() {}

void Settings::selectPage(int page) {
	ui->stackedWidget->setCurrentIndex(page);
}

void Settings::retranslateUi() {
	for(int page = 0; page < pager->page_count(); page++) {
		pager->set_text(page, page_name((Page)page));
	}
	ui->retranslateUi(this);
	ui->version_label->setText(ui->version_label->text().arg(LV_APPVER));
}

void Settings::handleControlJson(QJsonObject control_json) {
	control_json_dynamic = control_json;
}

void Settings::init_ui() {
	ui->setupUi(this);
	init_selector();
	ui->box_startup->setVisible(startup_interface->isSupported());
	ui->box_update->setVisible(dynamic_cast<MainWindow*>(parent())->client_.updater_->supportsUpdate());
}

void Settings::reset_ui_states() {
	/* GUI-related settings */
	ui->box_startup->setChecked(startup_interface->isEnabled());
	ui->box_update->setChecked(dynamic_cast<MainWindow*>(parent())->client_.updater_->enabled());

	/* Daemon-related settings */
	control_json_static = control_json_dynamic; // "Fixing" a version of control_json

	QJsonObject client = control_json_static["globals"].toObject();

	// client_name
	ui->line_device_name->setText(client["client_name"].toString());

	// p2p_listen
	p2p_listen.setAuthority(client["p2p_listen"].toString());
	ui->port_box->setChecked(p2p_listen.port(0) != 0);
	ui->port_value->setEnabled(p2p_listen.port(0) != 0);
	ui->port_value->setValue(p2p_listen.port(0));

	// natpmp_enabled
	ui->natpmp_box->setChecked(client["natpmp_enabled"].toBool());

	// bttracker_enabled
	ui->global_discovery_box->setChecked(client["bttracker_enabled"].toBool());

	// multicast4_enabled || multicast6_enabled
	ui->local_discovery_box->setChecked(
		client["multicast4_enabled"].toBool()
			|| client["multicast6_enabled"].toBool()
	);
}

void Settings::process_ui_states() {
	/* GUI-related settings */
	startup_interface->setEnabled(ui->box_startup->isChecked());
	dynamic_cast<MainWindow*>(parent())->client_.updater_->setEnabled(ui->box_update->isChecked());

	/* Daemon-related settings */
	QJsonObject client;

	// client_name
	client["client_name"] = ui->line_device_name->text();

	// p2p_listen
	p2p_listen.setPort(ui->port_box->isChecked() ? ui->port_value->value() : 0);
	client["p2p_listen"] = p2p_listen.authority();

	// natpmp_enabled
	client["natpmp_enabled"] = ui->natpmp_box->isChecked();

	// bttracker_enabled
	client["bttracker_enabled"] = ui->global_discovery_box->isChecked();

	// multicast4_enabled || multicast6_enabled
	client["multicast4_enabled"] = ui->local_discovery_box->isChecked();
	client["multicast6_enabled"] = ui->local_discovery_box->isChecked();

	emit newConfigIssued(client);
}

void Settings::init_selector() {
	int page;

	pager = new Pager(ui->controlBar, this);

	page = pager->add_page();
	pager->set_icon(page, GUIIconProvider::get_instance()->get_icon(GUIIconProvider::SETTINGS_GENERAL));
	//page = pager->add_page();
	//pager->set_icon(page, GUIIconProvider::get_instance()->get_icon(GUIIconProvider::SETTINGS_ACCOUNT));
	page = pager->add_page();
	pager->set_icon(page, GUIIconProvider::get_instance()->get_icon(GUIIconProvider::SETTINGS_NETWORK));
	//page = pager->add_page();
	//pager->set_icon(page, GUIIconProvider::get_instance()->get_icon(GUIIconProvider::SETTINGS_ADVANCED));

	pager->show();

	connect(pager, &Pager::pageSelected, this, &Settings::selectPage);
}

QString Settings::page_name(Page page) {
	switch(page) {
		case Page::PAGE_GENERAL:
			return QApplication::translate("Settings", "General", 0);
		//case Page::PAGE_ACCOUNT:
		//	return QApplication::translate("Settings", "Account", 0);
		case Page::PAGE_NETWORK:
			return QApplication::translate("Settings", "Network", 0);
		//case Page::PAGE_ADVANCED:
		//	return QApplication::translate("Settings", "Advanced", 0);
		default:
			return QString();
	}
}

void Settings::showEvent(QShowEvent* e) {
	QDialog::showEvent(e);
	reset_ui_states();
}

void Settings::okayPressed() {
	process_ui_states();
	close();
	reset_ui_states();
}
void Settings::applyPressed() {
	process_ui_states();
}
void Settings::cancelPressed() {
	close();
	reset_ui_states();
}
