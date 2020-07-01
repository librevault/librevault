#pragma once
#include "GenericRemoteDictionary.h"

class Daemon;

class RemoteState : public GenericRemoteDictionary {
  Q_OBJECT

 public:
  explicit RemoteState(Daemon* daemon);
  virtual ~RemoteState() {}
};
