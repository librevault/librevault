#pragma once
#include <QFile>
#include <QObject>
#include <QSslCertificate>
#include <QSslKey>

#include "util/log.h"

Q_DECLARE_LOGGING_CATEGORY(log_nodekey)

namespace librevault {

class NodeKey : public QObject {
  Q_OBJECT
  LOG_SCOPE("NodeKey");

 public:
  NodeKey(QObject* parent);
  ~NodeKey() override;

  QCryptographicHash::Algorithm digestAlgorithm() const { return QCryptographicHash::Sha256; }

  QByteArray digest() const;
  QSslKey privateKey() const { return private_key_; }
  QSslCertificate certificate() const { return certificate_; }

 private:
  void writeKey();
  void genCertificate();

  QFile cert_file_, private_key_file_;
  QSslKey private_key_;
  QSslCertificate certificate_;
};

}  // namespace librevault
