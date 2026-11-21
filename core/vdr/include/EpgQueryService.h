#pragma once

#include "IEpgQueryService.h"
#include "VdrService.h"

class EpgQueryService : public IEpgQueryService {
public:
    explicit EpgQueryService(VdrService& vdrService);

    std::vector<VdrEvent> getNowNext(
        const std::string& channelId,
        int from) const override;

    std::vector<VdrEvent> getTimeWindow(
        const std::string& channelId,
        int from,
        int timespan) const override;

    std::vector<VdrEvent> getChannelWindow(
        const std::string& channelId,
        int from,
        int timespan,
        int limit) const override;

private:
    VdrService& vdrService_;
};
