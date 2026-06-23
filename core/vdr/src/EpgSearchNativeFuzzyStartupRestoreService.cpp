#include "EpgSearchNativeFuzzyStartupRestoreService.h"

#include "BackendNode.h"
#include "BackendRegistryService.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"

EpgSearchNativeFuzzyStartupRestoreService::EpgSearchNativeFuzzyStartupRestoreService(
    EpgSearchNativeFuzzyCapabilityRepository& repository,
    EpgSearchNativeFuzzyCapabilityDetector& detector,
    BackendRegistryService& backendRegistryService)
    : repository_(repository),
      detector_(detector),
      backendRegistryService_(backendRegistryService)
{
}

EpgSearchNativeFuzzyStartupRestoreSummary
EpgSearchNativeFuzzyStartupRestoreService::restoreAllBackends()
{
    EpgSearchNativeFuzzyStartupRestoreSummary summary;

    if (!repository_.ensureSchema())
    {
        return summary;
    }

    summary.schemaReady = true;

    const auto backends = backendRegistryService_.listBackends();

    for (const BackendNode& backend : backends)
    {
        ++summary.backendsSeen;

        const auto persistedProbeResult = repository_.load(
            backend.backendId);

        if (!persistedProbeResult.has_value())
        {
            continue;
        }

        ++summary.persistedResultsFound;

        const bool nativeAvailable = detector_.nativeFuzzyAvailable(
            *persistedProbeResult);

        if (nativeAvailable)
        {
            ++summary.nativeFuzzyAvailable;
        }

        const auto detectedCapabilities = detector_.apply(
            backend.capabilities,
            *persistedProbeResult);

        if (backendRegistryService_.updateBackendCapabilities(
                backend.backendId,
                detectedCapabilities))
        {
            ++summary.backendsUpdated;
        }
    }

    return summary;
}
