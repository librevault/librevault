#pragma once
#include <natpmp.h>

#include <QLoggingCategory>
#include <QTimer>

#include "PortMappingSubService.h"
#include "util/log.h"

Q_DECLARE_LOGGING_CATEGORY(log_natpmp)

namespace librevault {

class P2PProvider;
class NATPMPMapping;
class NATPMPService : public PortMappingSubService {
  friend class NATPMPMapping;

 public:
  NATPMPService(PortMappingService& parent);
  virtual ~NATPMPService();

  void start();
  void stop();

  void map(const MappingRequest& request);
  void unmap(const QString& id);

 protected:
  bool active_;
  natpmp_t natpmp;

  QHash<QString, MappingRequest> requests_;
  std::map<QString, std::unique_ptr<NATPMPMapping>> mappings_;
};

class NATPMPMapping : public QObject {
  Q_OBJECT
 public:
  NATPMPMapping(NATPMPService& parent, MappingRequest request);
  ~NATPMPMapping();

 private:
  NATPMPService& parent_;
  MappingRequest request_;

  QTimer* timer_;

 private slots:
  void sendPeriodicRequest();
  void sendZeroRequest();
};

}  // namespace librevault
