#pragma once
#include <QObject>

#include "PortMappingService.h"

namespace librevault {

class PortMappingSubService : public QObject {
  Q_OBJECT
 public:
  explicit PortMappingSubService(PortMappingService &parent) : QObject(&parent), parent_(parent) {}

  Q_SIGNAL void portMapped(MappingResult result);

  virtual void map(const MappingRequest &descriptor) = 0;
  virtual void unmap(const QString &id) = 0;

 protected:
  PortMappingService &parent_;

  inline void add_existing() { parent_.add_existing_mappings(this); }
};

}  // namespace librevault
