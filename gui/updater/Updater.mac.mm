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

#ifdef BUILD_UPDATER

#include "Updater.h"
#include <Sparkle/Sparkle.h>
#include <Sparkle/SUUpdater.h>

struct Updater::Impl {
	SUUpdater* updater;
};

Updater::Updater(QObject* parent) : QObject(parent), impl_(new Impl()) {
	impl_->updater = [SUUpdater sharedUpdater];

	QString appcast_url = QStringLiteral("https://releases.librevault.com/appcast_mac.rss");
	NSURL* appcast_nsurl = [NSURL URLWithString:
	[NSString stringWithUTF8String: appcast_url.toUtf8().data()]
	];

	[impl_->updater setFeedURL: appcast_nsurl];
	[impl_->updater setAutomaticallyChecksForUpdates:YES];
	[impl_->updater setAutomaticallyDownloadsUpdates:NO];
	[impl_->updater setSendsSystemProfile:NO];
	[impl_->updater resetUpdateCycle];
	[impl_->updater retain];

	checkUpdatesSilently();
}

Updater::~Updater() {/*
	[impl_->updater release];
	delete impl_;*/
}

bool Updater::supportsUpdate() const {
	return true;
}

bool Updater::enabled() const {
	return [impl_->updater automaticallyChecksForUpdates];
}

void Updater::checkUpdates() {
	[impl_->updater checkForUpdates: NSApp];
}

void Updater::checkUpdatesSilently() {
	[impl_->updater checkForUpdatesInBackground];
}

void Updater::setEnabled(bool enable) {
	[impl_->updater setAutomaticallyChecksForUpdates: enable];
}

#endif	/* BUILD_UPDATER */
