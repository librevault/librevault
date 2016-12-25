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
#include "Settings.h"
#include "control/Daemon.h"
#include "control/RemoteConfig.h"
#include "icons/GUIIconProvider.h"
#include "updater/Updater.h"
#include "MainWindow.h"
#include "appver.h"
#include <QCloseEvent>
#include <QDebug>

Settings::Settings(Daemon* daemon, Updater* updater, QWidget* parent) :
		QDialog(parent),
		daemon_(daemon),
		updater_(updater) {
	startup_interface_ = new StartupInterface(this);

	init_ui();
	connect(ui.dialog_box, &QDialogButtonBox::accepted, this, &Settings::okayPressed);
	connect(ui.dialog_box->button(QDialogButtonBox::StandardButton::Apply), &QPushButton::clicked, this, &Settings::applyPressed);
	connect(ui.dialog_box, &QDialogButtonBox::rejected, this, &Settings::cancelPressed);
}

Settings::~Settings() {}

void Settings::retranslateUi() {
	for(int page = 0; page < pager->page_count(); page++) {
		pager->set_text(page, page_name((Page)page));
	}
	ui.retranslateUi(this);
	ui.version_label->setText(ui.version_label->text().arg(LV_APPVER));
}

void Settings::init_ui() {
	ui.setupUi(this);
	init_selector();
	ui.box_startup->setVisible(startup_interface_->isSupported());
	ui.box_update->setVisible(updater_->supportsUpdate());
}

void Settings::reset_ui_states() {
	/* GUI-related settings */
	ui.box_startup->setChecked(startup_interface_->isEnabled());
	ui.box_update->setChecked(updater_->enabled());

	// client_name
	ui.line_device_name->setText(daemon_->config()->getGlobalValue("client_name").toString());

	// p2p_listen
	unsigned short p2p_listen = daemon_->config()->getGlobalValue("p2p_listen").toInt();
	ui.port_box->setChecked(p2p_listen != 0);
	ui.port_value->setEnabled(p2p_listen != 0);
	ui.port_value->setValue(p2p_listen);

	// natpmp_enabled
	ui.natpmp_box->setChecked(daemon_->config()->getGlobalValue("natpmp_enabled").toBool());

	// upnp_enabled
	ui.upnp_box->setChecked(daemon_->config()->getGlobalValue("upnp_enabled").toBool());

	// bttracker_enabled
	ui.global_discovery_box->setChecked(daemon_->config()->getGlobalValue("bttracker_enabled").toBool());

	// multicast4_enabled || multicast6_enabled
	ui.local_discovery_box->setChecked(
		daemon_->config()->getGlobalValue("multicast4_enabled").toBool()
			|| daemon_->config()->getGlobalValue("multicast6_enabled").toBool()
	);

	// mainline_dht_enabled
	ui.dht_discovery_box->setChecked(daemon_->config()->getGlobalValue("mainline_dht_enabled").toBool());
}

void Settings::process_ui_states() {
	/* GUI-related settings */
	startup_interface_->setEnabled(ui.box_startup->isChecked());
	updater_->setEnabled(ui.box_update->isChecked());

	/* Daemon-related settings */
	// client_name
	daemon_->config()->setGlobalValue("client_name", ui.line_device_name->text());

	// p2p_listen
	daemon_->config()->setGlobalValue("p2p_listen", ui.port_box->isChecked() ? ui.port_value->value() : 0);

	// natpmp_enabled
	daemon_->config()->setGlobalValue("natpmp_enabled", ui.natpmp_box->isChecked());

	// upnp_enabled
	daemon_->config()->setGlobalValue("upnp_enabled", ui.upnp_box->isChecked());

	// bttracker_enabled
	daemon_->config()->setGlobalValue("bttracker_enabled", ui.global_discovery_box->isChecked());

	// multicast4_enabled || multicast6_enabled
	daemon_->config()->setGlobalValue("multicast4_enabled", ui.local_discovery_box->isChecked());
	daemon_->config()->setGlobalValue("multicast6_enabled", ui.local_discovery_box->isChecked());

	// mainline_dht_enabled
	daemon_->config()->setGlobalValue("mainline_dht_enabled", ui.dht_discovery_box->isChecked());
}

void Settings::init_selector() {
	int page;

	pager = new Pager(ui.controlBar, this);

	page = pager->add_page();
	pager->set_icon(page, GUIIconProvider().icon(GUIIconProvider::SETTINGS_GENERAL));
	//page = pager->add_page();
	//pager->set_icon(page, GUIIconProvider().icon(GUIIconProvider::SETTINGS_ACCOUNT));
	page = pager->add_page();
	pager->set_icon(page, GUIIconProvider().icon(GUIIconProvider::SETTINGS_NETWORK));
	//page = pager->add_page();
	//pager->set_icon(page, GUIIconProvider().icon(GUIIconProvider::SETTINGS_ADVANCED));

	pager->show();

	connect(pager, &Pager::pageSelected, ui.stackedWidget, &QStackedWidget::setCurrentIndex);
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
