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
}

void Settings::handleControlJson(QJsonObject control_json) {
	control_json_dynamic = control_json;
}

void Settings::init_ui() {
	ui->setupUi(this);
	init_selector();
}

void Settings::reset_ui_states() {
	control_json_static = control_json_dynamic;
	QJsonObject
		config,
		config_net,
		config_net_natpmp,
		config_discovery,
		config_discovery_bttracker,
		config_discovery_multicast4,
		config_discovery_multicast6;
	config = control_json_static["config"].toObject();
	config_net = config["net"].toObject();
	config_net_natpmp = config_net["natpmp"].toObject();
	config_discovery = config["discovery"].toObject();
	config_discovery_bttracker = config_discovery["bttracker"].toObject();
	config_discovery_multicast4 = config_discovery["multicast4"].toObject();
	config_discovery_multicast6 = config_discovery["multicast6"].toObject();

	// net.listen
	net_listen.setAuthority(config_net["listen"].toString());
	ui->port_box->setChecked(net_listen.port(0) != 0);
	ui->port_value->setEnabled(net_listen.port(0) != 0);
	ui->port_value->setValue(net_listen.port(0));

	// net.natpmp
	ui->natpmp_box->setChecked(get_json_string_as_bool(config_net_natpmp["enabled"]));

	// discovery.bttracker
	ui->global_discovery_box->setChecked(get_json_string_as_bool(config_discovery_bttracker["enabled"]));
	// discovery.multicastX
	ui->local_discovery_box->setChecked(
		get_json_string_as_bool(config_discovery_multicast4["enabled"])
			|| get_json_string_as_bool(config_discovery_multicast6["enabled"])
	);

	// Non-daemon-related settings
	ui->box_startup->setChecked(startup_interface->isEnabled());
}

bool Settings::get_json_string_as_bool(QJsonValueRef val) const {
	if(val.isString())
		return val.toString() == "true";
	else
		return val.toBool();
}

void Settings::process_ui_states() {
	QJsonObject
		config,
		config_net,
		config_net_natpmp,
		config_discovery,
		config_discovery_bttracker,
		config_discovery_multicast4,
		config_discovery_multicast6;

	/* config.net */
	// net.listen
	net_listen.setPort(ui->port_box->isChecked() ? ui->port_value->value() : 0);
	config_net["listen"] = net_listen.authority();
	// net.natpmp
	bool natpmp_enabled = ui->natpmp_box->isChecked();
	config_net_natpmp["enabled"] = natpmp_enabled;

	/* config.discovery */
	// discovery.bttracker
	bool discovery_global_enabled = ui->global_discovery_box->isChecked();
	config_discovery_bttracker["enabled"] = discovery_global_enabled;

	bool discovery_local_enabled = ui->local_discovery_box->isChecked();
	config_discovery_multicast4["enabled"] = discovery_local_enabled;
	config_discovery_multicast6["enabled"] = discovery_local_enabled;

	// Constructing JSON (in reverse)
	config_discovery["multicast6"] = config_discovery_multicast6;
	config_discovery["multicast4"] = config_discovery_multicast4;
	config_discovery["bttracker"] = config_discovery_bttracker;
	config["discovery"] = config_discovery;
	config_net["natpmp"] = config_net_natpmp;
	config["net"] = config_net;

	emit newConfigIssued(config);

	// Non-daemon-related settings
	startup_interface->setEnabled(ui->box_startup->isChecked());
}

void Settings::init_selector() {
	int page;

	pager = new Pager(ui->controlBar, parentWidget());

	page = pager->add_page();
	pager->set_icon(page, page_icon((Page)page));
	page = pager->add_page();
	pager->set_icon(page, page_icon((Page)page));
	page = pager->add_page();
	pager->set_icon(page, page_icon((Page)page));
	page = pager->add_page();
	pager->set_icon(page, page_icon((Page)page));

	connect(pager, &Pager::pageSelected, this, &Settings::selectPage);
}

QString Settings::page_name(Page page) {
	switch(page) {
		case Page::PAGE_GENERAL:
			return QApplication::translate("Settings", "General", 0);
		case Page::PAGE_ACCOUNT:
			return QApplication::translate("Settings", "Account", 0);
		case Page::PAGE_NETWORK:
			return QApplication::translate("Settings", "Network", 0);
		case Page::PAGE_ADVANCED:
			return QApplication::translate("Settings", "Advanced", 0);
	}
}

QIcon Settings::page_icon(Page page) {
	QIcon icon;
	if(page == Page::PAGE_GENERAL) {
		icon.addFile(QStringLiteral(":/branding/librevault_icon.svg"), QSize(), QIcon::Normal, QIcon::Off);
	}else if(page == Page::PAGE_ACCOUNT){
		QString iconThemeName = QStringLiteral("user-identity");
		if(QIcon::hasThemeIcon(iconThemeName))
			icon = QIcon::fromTheme(iconThemeName);
		else
			icon.addFile(QStringLiteral(":/icons/ic_account_box_black_48px.svg"), QSize(), QIcon::Normal, QIcon::Off);
	}else if(page == Page::PAGE_NETWORK){
		QString iconThemeName = QStringLiteral("network-wireless");
		if(QIcon::hasThemeIcon(iconThemeName))
			icon = QIcon::fromTheme(iconThemeName);
		else
			icon.addFile(QStringLiteral(":/icons/ic_settings_ethernet_black_48px.svg"), QSize(), QIcon::Normal, QIcon::Off);
	}else if(page == Page::PAGE_ADVANCED){
		QString iconThemeName = QStringLiteral("document-properties");
		if(QIcon::hasThemeIcon(iconThemeName))
			icon = QIcon::fromTheme(iconThemeName);
		else
			icon.addFile(QStringLiteral(":/icons/ic_settings_black_48px.svg"), QSize(), QIcon::Normal, QIcon::Off);
	}
	return icon;
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
