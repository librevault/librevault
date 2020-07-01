#include "RemoteState.h"

RemoteState::RemoteState(Daemon* daemon)
    : GenericRemoteDictionary(daemon, "/v1/state", "/v1/folders/state", "EVENT_GLOBAL_STATE_CHANGED",
                              "EVENT_FOLDER_CONFIG_CHANGED") {}
