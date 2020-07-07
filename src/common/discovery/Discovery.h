#pragma once
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>

#include "DiscoveryResult.h"

namespace librevault {

class FolderGroup;

class MulticastProvider;
class MLDHTProvider;
class TrackerProvider;

class NodeKey;
class PortMappingService;
class StateCollector;

class Discovery : public QObject {
  Q_OBJECT

 signals:
  void discovered(QByteArray folderid, DiscoveryResult result);

 public:
  Discovery(const QByteArray& node_id, PortMappingService* port_mapping, StateCollector* state_collector,
            QNetworkAccessManager* nam, QObject* parent);
  virtual ~Discovery();

 public slots:
  void addGroup(FolderGroup* fgroup);
  void removeGroup(const QByteArray& groupid);

 protected:
  MulticastProvider* multicast_;
  MLDHTProvider* mldht_;
  TrackerProvider* tracker_;
};

}  // namespace librevault