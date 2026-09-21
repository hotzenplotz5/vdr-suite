#pragma once

#include "VdrEvent.h"

#include <string>
#include <vector>

class IEpgQueryService {
public:
    virtual ~IEpgQueryService() = default;

    virtual std::vector<VdrEvent> getNowNext(
        const std::string& channelId,
        int from) const = 0;

    virtual std::vector<VdrEvent> getTimeWindow(
        const std::string& channelId,
        int from,
        int timespan) const = 0;

    virtual std::vector<VdrEvent> getChannelWindow(
        const std::string& channelId,
        int from,
        int timespan,
        int limit) const = 0;
};
