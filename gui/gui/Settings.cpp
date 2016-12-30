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

Settings::Settings(Daemon* daemon, Updater* updater, QWidget* parent) :
		SettingsWindow(parent),
		daemon_(daemon),
		updater_(updater) {
	startup_interface_ = new StartupInterface(this);

	init_ui();
	connect(ui_bottom_.dialog_box, &QDialogButtonBox::accepted, this, &Settings::okayPressed);
	connect(ui_bottom_.dialog_box, &QDialogButtonBox::rejected, this, &Settings::cancelPressed);

	reset_ui_states();
}

Settings::~Settings() {}

void Settings::retranslateUi() {
	ui_pane_general_.retranslateUi(pane_general_);
	ui_pane_network_.retranslateUi(pane_network_);
	ui_bottom_.bottom_label->setText(tr("Version: %1").arg(LV_APPVER));
}

void Settings::init_ui() {
	pane_general_ = new QWidget();
	ui_pane_general_.setupUi(pane_general_);
	pane_general_->setWindowIcon(GUIIconProvider().icon(GUIIconProvider::SETTINGS_GENERAL));
	addPane(pane_general_);

	pane_network_ = new QWidget();
	ui_pane_network_.setupUi(pane_network_);
	pane_network_->setWindowIcon(GUIIconProvider().icon(GUIIconProvider::SETTINGS_NETWORK));
	addPane(pane_network_);

	ui_pane_general_.box_startup->setVisible(startup_interface_->isSupported());
	ui_pane_general_.box_update->setVisible(updater_->supportsUpdate());
}

void Settings::reset_ui_states() {
	/* GUI-related settings */
	ui_pane_general_.box_startup->setChecked(startup_interface_->isEnabled());
	ui_pane_general_.box_update->setChecked(updater_->enabled());

	/* Daemon settings */
	ui_pane_general_.line_device->setText(daemon_->config()->getGlobalValue("client_name").toString()); // client_name

	// p2p_download_slots
	ui_pane_network_.spin_down_slots->setValue(daemon_->config()->getGlobalValue("p2p_download_slots").toInt());

	// p2p_listen
	unsigned short p2p_listen = daemon_->config()->getGlobalValue("p2p_listen").toInt();
	ui_pane_network_.box_port_random->setChecked(p2p_listen == 0);
	ui_pane_network_.spin_port->setEnabled(p2p_listen != 0);
	ui_pane_network_.spin_port->setValue(p2p_listen);

	// natpmp_enabled
	ui_pane_network_.natpmp_box->setChecked(daemon_->config()->getGlobalValue("natpmp_enabled").toBool());

	// upnp_enabled
	ui_pane_network_.upnp_box->setChecked(daemon_->config()->getGlobalValue("upnp_enabled").toBool());

	// bttracker_enabled
	ui_pane_network_.global_discovery_box->setChecked(daemon_->config()->getGlobalValue("bttracker_enabled").toBool());

	// multicast4_enabled || multicast6_enabled
	ui_pane_network_.local_discovery_box->setChecked(
		daemon_->config()->getGlobalValue("multicast4_enabled").toBool() ||
		daemon_->config()->getGlobalValue("multicast6_enabled").toBool()
	);

	// mainline_dht_enabled
	ui_pane_network_.dht_discovery_box->setChecked(daemon_->config()->getGlobalValue("mainline_dht_enabled").toBool());
}

void Settings::process_ui_states() {
	/* GUI-related settings */
	startup_interface_->setEnabled(ui_pane_general_.box_startup->isChecked());
	updater_->setEnabled(ui_pane_general_.box_update->isChecked());

	/* Daemon settings */
	// client_name
	daemon_->config()->setGlobalValue("client_name", ui_pane_general_.line_device->text());

	// p2p_download_slots
	daemon_->config()->setGlobalValue("p2p_download_slots", ui_pane_network_.spin_down_slots->value());

	// p2p_listen
	daemon_->config()->setGlobalValue("p2p_listen", ui_pane_network_.box_port_random->isChecked() ? 0 : ui_pane_network_.spin_port->value());

	// natpmp_enabled
	daemon_->config()->setGlobalValue("natpmp_enabled", ui_pane_network_.natpmp_box->isChecked());

	// upnp_enabled
	daemon_->config()->setGlobalValue("upnp_enabled", ui_pane_network_.upnp_box->isChecked());

	// bttracker_enabled
	daemon_->config()->setGlobalValue("bttracker_enabled", ui_pane_network_.global_discovery_box->isChecked());

	// multicast4_enabled || multicast6_enabled
	daemon_->config()->setGlobalValue("multicast4_enabled", ui_pane_network_.local_discovery_box->isChecked());
	daemon_->config()->setGlobalValue("multicast6_enabled", ui_pane_network_.local_discovery_box->isChecked());

	// mainline_dht_enabled
	daemon_->config()->setGlobalValue("mainline_dht_enabled", ui_pane_network_.dht_discovery_box->isChecked());
}

void Settings::okayPressed() {
	process_ui_states();
	close();
}
void Settings::cancelPressed() {
	close();
}

void Settings::showEvent(QShowEvent* e) {
	reset_ui_states();
	QDialog::showEvent(e);
}
