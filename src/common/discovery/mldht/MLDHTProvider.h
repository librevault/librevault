#pragma once
#include <QHostInfo>
#include <QLoggingCategory>
#include <QTimer>
#include <QUdpSocket>

#include "discovery/DiscoveryResult.h"
#include "discovery/btcompat.h"

Q_DECLARE_LOGGING_CATEGORY(log_dht)

extern "C" {
extern void lv_dht_callback_glue(void* closure, int event, const unsigned char* info_hash, const void* data,
                                 size_t data_len);
}

namespace librevault {

class PortMappingService;
class StateCollector;
class MLDHTProvider : public QObject {
  Q_OBJECT
 public:
  MLDHTProvider(PortMappingService* port_mapping, QObject* parent);
  virtual ~MLDHTProvider();

  void passCallback(void* closure, int event, const uint8_t* info_hash, const QByteArray& data);

  [[nodiscard]] int nodeCount() const;

  [[nodiscard]] quint16 getPort();
  [[nodiscard]] quint16 getExternalPort();

 signals:
  void eventReceived(int event, btcompat::info_hash ih, QByteArray values);
  void discovered(QByteArray folderid, DiscoveryResult result);
  void nodeCountChanged(int count);

 public slots:
  void addNode(const Endpoint& endpoint);

 private:
  PortMappingService* port_mapping_;

  using dht_id = btcompat::info_hash;
  dht_id own_id;

  // Sockets
  QUdpSocket* socket_;
  QTimer* periodic_;

  // Initialization
  void init();
  void deinit();

  void readSessionFile();
  void writeSessionFile();

  void processDatagram();
  void periodicRequest();
};

}  // namespace librevault
