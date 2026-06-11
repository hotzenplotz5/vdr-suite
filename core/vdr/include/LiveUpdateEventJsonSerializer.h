#ifndef LIVE_UPDATE_EVENT_JSON_SERIALIZER_H
#define LIVE_UPDATE_EVENT_JSON_SERIALIZER_H

#include "LiveUpdateEvent.h"

#include <string>

class LiveUpdateEventJsonSerializer {
public:
    std::string serializeEvent(
        const LiveUpdateEvent& event) const;
};

#endif
