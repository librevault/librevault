#ifndef BUILD_UPDATER

#include "Updater.h"

Updater::Updater(QObject* parent) : QObject(parent) {}

Updater::~Updater() {}

bool Updater::supportsUpdate() const { return false; }
bool Updater::enabled() const { return false; }
void Updater::checkUpdates() {}
void Updater::checkUpdatesSilently() {}
void Updater::setEnabled(bool enable) {}

#endif /* BUILD_UPDATER */
