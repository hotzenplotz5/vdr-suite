#pragma once

#include "DashboardController.h"

class MediaPlaybackContractResponse
{
public:
    static ApiResponse augment(ApiResponse response, bool liveResource);
};
