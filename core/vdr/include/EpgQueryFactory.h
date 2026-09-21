#pragma once

#include "EpgQueryRequest.h"
#include "EpgQueryScope.h"
#include "VdrEventQuery.h"

#include <string>

class EpgQueryFactory {
public:
    static VdrEventQuery create(const EpgQueryRequest& request)
    {
        switch (request.scope) {
        case EpgQueryScope::NowNext:
            return forChannelNowNext(request.channelId, request.from);
        case EpgQueryScope::TimeWindow:
            return forChannelTimeWindow(
                request.channelId,
                request.from,
                request.timespan);
        case EpgQueryScope::ChannelWindow:
            return forChannelWindow(
                request.channelId,
                request.from,
                request.timespan,
                request.limit);
        }

        return VdrEventQuery();
    }

    static VdrEventQuery forChannelNowNext(
        const std::string& channelId,
        int from)
    {
        VdrEventQuery query;
        query.channelId = channelId;
        query.from = from;
        query.timespan = 7200;
        query.channelEventLimit = 2;

        return query;
    }

    static VdrEventQuery forChannelTimeWindow(
        const std::string& channelId,
        int from,
        int timespan)
    {
        VdrEventQuery query;
        query.channelId = channelId;
        query.from = from;
        query.timespan = timespan;

        return query;
    }

    static VdrEventQuery forChannelWindow(
        const std::string& channelId,
        int from,
        int timespan,
        int limit)
    {
        VdrEventQuery query = forChannelTimeWindow(channelId, from, timespan);
        query.limit = limit;

        return query;
    }
};
