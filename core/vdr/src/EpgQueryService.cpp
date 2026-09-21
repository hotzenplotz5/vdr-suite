#include "EpgQueryService.h"

#include "EpgQueryFactory.h"

EpgQueryService::EpgQueryService(VdrService& vdrService)
    : vdrService_(vdrService)
{
}

std::vector<VdrEvent> EpgQueryService::getNowNext(
    const std::string& channelId,
    int from) const
{
    EpgQueryRequest request;
    request.scope = EpgQueryScope::NowNext;
    request.channelId = channelId;
    request.from = from;

    return vdrService_.getEvents(EpgQueryFactory::create(request));
}

std::vector<VdrEvent> EpgQueryService::getTimeWindow(
    const std::string& channelId,
    int from,
    int timespan) const
{
    EpgQueryRequest request;
    request.scope = EpgQueryScope::TimeWindow;
    request.channelId = channelId;
    request.from = from;
    request.timespan = timespan;

    return vdrService_.getEvents(EpgQueryFactory::create(request));
}

std::vector<VdrEvent> EpgQueryService::getChannelWindow(
    const std::string& channelId,
    int from,
    int timespan,
    int limit) const
{
    EpgQueryRequest request;
    request.scope = EpgQueryScope::ChannelWindow;
    request.channelId = channelId;
    request.from = from;
    request.timespan = timespan;
    request.limit = limit;

    return vdrService_.getEvents(EpgQueryFactory::create(request));
}
