#pragma once

#include "DashboardController.h"

class ILiveTransport;

class LiveTransportController
{
public:
    explicit LiveTransportController(
        const ILiveTransport& transport);

    ApiResponse getStream();

private:
    const ILiveTransport& transport_;
};
