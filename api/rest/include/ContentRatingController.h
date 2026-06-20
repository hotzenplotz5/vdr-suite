#pragma once

#include "DashboardController.h"

class ContentRatingResolutionJsonSerializer;
class ContentRatingResolutionResult;

class ContentRatingController
{
public:
    explicit ContentRatingController(
        ContentRatingResolutionJsonSerializer& jsonSerializer);

    ApiResponse getRatingResolution(
        const ContentRatingResolutionResult& result);

private:
    ContentRatingResolutionJsonSerializer& jsonSerializer_;
};
