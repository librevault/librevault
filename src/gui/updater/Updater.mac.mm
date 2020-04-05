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
	[impl_->updater setAutomaticallyDownloadsUpdates:NO];
	[impl_->updater setSendsSystemProfile:NO];
	[impl_->updater resetUpdateCycle];

	checkUpdatesSilently();
}

Updater::~Updater() {
	delete impl_;
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
