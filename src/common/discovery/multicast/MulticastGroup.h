#pragma once
#include <util/Endpoint.h>

#include <QTimer>
#include <QUdpSocket>

namespace librevault {

class FolderGroup;
class MulticastProvider;
class MulticastGroup : public QObject {
  Q_OBJECT

 public:
  MulticastGroup(MulticastProvider* provider, QByteArray groupid, QObject* parent);

  void setEnabled(bool enabled);

 private:
  MulticastProvider* provider_;
  QByteArray groupid_;

  QTimer* timer_;

  QByteArray message_;

  QByteArray getMessage();
  void sendMulticast(QUdpSocket* socket, const Endpoint& endpoint);

 private slots:
  void sendMulticasts();
};

}  // namespace librevault
