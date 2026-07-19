#include "EpgArtworkEnrichmentService.h"

EpgArtworkEnrichmentService::EpgArtworkEnrichmentService(
    EpgArtworkRepository& repository,
    IEpgArtworkResolver& resolver)
    : repository_(repository),
      resolver_(resolver)
{
}

EpgArtworkEnrichmentResult EpgArtworkEnrichmentService::enrich(
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    EpgArtworkEnrichmentResult result;

    for (const VdrEvent& event : events)
    {
        if (event.id.empty() || event.channelId.empty())
        {
            continue;
        }

        const EpgArtworkResolution resolution = resolver_.resolve(
            backendId,
            event);

        if (!resolution.attempted)
        {
            ++result.unavailable;
            continue;
        }

        ++result.attempted;

        if (!resolution.found)
        {
            if (repository_.removeForEvent(
                    backendId,
                    event.channelId,
                    event.id))
            {
                ++result.removed;
            }
            else
            {
                result.repositoryOk = false;
            }
            continue;
        }

        EpgArtworkReference artwork = resolution.artwork;
        artwork.backendId = backendId;
        artwork.channelId = event.channelId;
        artwork.eventId = event.id;

        if (repository_.upsert(artwork))
        {
            ++result.stored;
        }
        else
        {
            result.repositoryOk = false;
        }
    }

    return result;
}
