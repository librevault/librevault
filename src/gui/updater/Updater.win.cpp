#ifdef BUILD_UPDATER

#include <winsparkle.h>

#include "Updater.h"

Updater::Updater(QObject* parent) : QObject(parent) {
  QString appcast_url = QStringLiteral("https://releases.librevault.com/appcast_win.rss");

  win_sparkle_set_appcast_url(appcast_url.toUtf8().data());
  win_sparkle_set_registry_path("Software\\Librevault\\Updates");
  win_sparkle_init();
}

Updater::~Updater() { win_sparkle_cleanup(); }

bool Updater::supportsUpdate() const { return true; }

bool Updater::enabled() const { return (bool)win_sparkle_get_automatic_check_for_updates(); }

void Updater::checkUpdates() { win_sparkle_check_update_with_ui(); }

void Updater::checkUpdatesSilently() { win_sparkle_check_update_without_ui(); }

void Updater::setEnabled(bool enable) { win_sparkle_set_automatic_check_for_updates(enable); }

#endif /* BUILD_UPDATER */
