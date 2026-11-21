#include "EpgSearchNativeFuzzyStartupRestoreService.h"

#include "BackendNode.h"
#include "BackendRegistryService.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityFreshnessPolicy.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"

EpgSearchNativeFuzzyStartupRestoreService::EpgSearchNativeFuzzyStartupRestoreService(
    EpgSearchNativeFuzzyCapabilityRepository& repository,
    EpgSearchNativeFuzzyCapabilityDetector& detector,
    BackendRegistryService& backendRegistryService,
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy& freshnessPolicy)
    : repository_(repository),
      detector_(detector),
      backendRegistryService_(backendRegistryService),
      freshnessPolicy_(freshnessPolicy)
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

        const auto persistedProbeResult = repository_.loadPersistedProbeResult(
            backend.backendId);

        if (!persistedProbeResult.has_value())
        {
            continue;
        }

        ++summary.persistedResultsFound;

        const auto freshnessDecision = freshnessPolicy_.decide(
            persistedProbeResult->ageSeconds);

        if (!freshnessDecision.fresh)
        {
            ++summary.staleResultsIgnored;

            VdrCapabilitySet staleSafeCapabilities = backend.capabilities;
            staleSafeCapabilities.epgSearchFuzzyNative = false;

            if (backendRegistryService_.updateBackendCapabilities(
                    backend.backendId,
                    staleSafeCapabilities))
            {
                ++summary.backendsUpdated;
            }

            continue;
        }

        const bool nativeAvailable = detector_.nativeFuzzyAvailable(
            persistedProbeResult->probeResult);

        if (nativeAvailable)
        {
            ++summary.nativeFuzzyAvailable;
        }

        const auto detectedCapabilities = detector_.apply(
            backend.capabilities,
            persistedProbeResult->probeResult);

        if (backendRegistryService_.updateBackendCapabilities(
                backend.backendId,
                detectedCapabilities))
        {
            ++summary.backendsUpdated;
        }
    }

    return summary;
}
