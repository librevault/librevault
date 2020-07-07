#pragma once
#include <QTimer>

#include "discovery/DiscoveryResult.h"
#include "discovery/btcompat.h"

namespace librevault {

class MLDHTProvider;
class FolderGroup;

class MLDHTGroup : public QObject {
  Q_OBJECT
 public:
  MLDHTGroup(MLDHTProvider *provider, const QByteArray &fgroup, QObject *parent);

  void setEnabled(bool enable);
  void startSearch(int af);

 signals:
  void discovered(DiscoveryResult result);

 public slots:
  void handleEvent(int event, btcompat::info_hash ih, QByteArray values);

 private:
  MLDHTProvider *provider_;
  QTimer *timer_;

  btcompat::info_hash info_hash_;
  QByteArray folderid_;

  bool enabled_ = false;
};

}  // namespace librevault
