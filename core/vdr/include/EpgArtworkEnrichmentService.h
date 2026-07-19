#pragma once

#include "EpgArtworkRepository.h"
#include "IEpgArtworkResolver.h"

#include <cstddef>
#include <string>
#include <vector>

struct EpgArtworkEnrichmentResult
{
    std::size_t attempted = 0;
    std::size_t stored = 0;
    std::size_t removed = 0;
    std::size_t unavailable = 0;
    bool repositoryOk = true;
};

class EpgArtworkEnrichmentService
{
public:
    EpgArtworkEnrichmentService(
        EpgArtworkRepository& repository,
        IEpgArtworkResolver& resolver);

    EpgArtworkEnrichmentResult enrich(
        const std::string& backendId,
        const std::vector<VdrEvent>& events);

private:
    EpgArtworkRepository& repository_;
    IEpgArtworkResolver& resolver_;
};
