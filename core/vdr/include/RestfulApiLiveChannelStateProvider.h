#pragma once

#include "IHttpClient.h"
#include "LiveOverlay.h"

class RestfulApiLiveChannelStateProvider : public ILiveChannelStateProvider
{
public:
    explicit RestfulApiLiveChannelStateProvider(IHttpClient& httpClient);

    LiveChannelState getState() const override;

    static std::string parseChannelId(const std::string& json);

private:
    IHttpClient& httpClient_;
};
