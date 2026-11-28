#ifndef LIVE_UPDATE_EVENT_JSON_SERIALIZER_H
#define LIVE_UPDATE_EVENT_JSON_SERIALIZER_H

#include "LiveUpdateEvent.h"

#include <string>
#include <vector>

class LiveUpdateEventJsonSerializer {
public:
    std::string serializeEvent(
        const LiveUpdateEvent& event,
        const std::vector<std::string>& recordingPublicIds = {}) const;
};

#endif
