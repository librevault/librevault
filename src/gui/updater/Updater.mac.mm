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
