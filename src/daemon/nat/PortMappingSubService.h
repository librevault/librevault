/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
