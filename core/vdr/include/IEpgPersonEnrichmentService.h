#pragma once

#include "VdrEvent.h"

#include <cstddef>
#include <string>
#include <vector>

struct EpgPersonEnrichmentResult
{
    std::size_t queued = 0;
    std::size_t deduplicated = 0;
    std::size_t suppressed = 0;
    std::size_t dropped = 0;
    bool queueAvailable = true;
};

class IEpgPersonEnrichmentService
{
public:
    virtual ~IEpgPersonEnrichmentService() = default;

    virtual EpgPersonEnrichmentResult enrich(
        const std::string& backendId,
        const std::vector<VdrEvent>& events) = 0;
};
