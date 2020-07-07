#include "RemoteFolder.h"

namespace librevault {

RemoteFolder::RemoteFolder(QObject* parent) : QObject(parent) {}
RemoteFolder::~RemoteFolder() {}

QString RemoteFolder::log_tag() const { return displayName(); }

/* InterestGuard */
RemoteFolder::InterestGuard::InterestGuard(RemoteFolder* remote) : remote_(remote) { remote_->interest(); }

RemoteFolder::InterestGuard::~InterestGuard() { remote_->uninterest(); }

std::shared_ptr<RemoteFolder::InterestGuard> RemoteFolder::get_interest_guard() {
  try {
    return std::shared_ptr<InterestGuard>(interest_guard_);
  } catch (std::bad_weak_ptr& e) {
    auto guard = std::make_shared<InterestGuard>(this);
    interest_guard_ = guard;
    return guard;
  }
}

}  // namespace librevault
