#pragma once

#include "SearchTimer.h"

#include <string>
#include <vector>

class RestfulApiSearchTimerMapper {
public:
    // RESTfulAPI supplies an unpaginated catalogue with count and total.
    // Reject malformed/partially decoded lists before lifecycle retirement.
    static bool completeCatalogue(const std::string& json,
        std::size_t parsedCount);

    static std::vector<SearchTimer> parseSearchTimers(
        const std::string& backendId,
        const std::string& json);
};