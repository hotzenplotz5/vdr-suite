#pragma once

#include "DashboardController.h"

#include <memory>
#include <string>

class ILiveTransport;
class BrowserLiveEventSubscription;

class LiveTransportController
{
public:
    explicit LiveTransportController(
        const ILiveTransport& transport);

    ApiResponse getStream(const std::string& lastEventId = "");
    std::shared_ptr<BrowserLiveEventSubscription> subscribeBrowser(
        const std::string& lastEventId) const;

private:
    const ILiveTransport& transport_;
};
