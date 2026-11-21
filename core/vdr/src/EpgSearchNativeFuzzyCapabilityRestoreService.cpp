#include "EpgSearchNativeFuzzyCapabilityRestoreService.h"

#include "BackendRegistryService.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"

EpgSearchNativeFuzzyCapabilityRestoreService::EpgSearchNativeFuzzyCapabilityRestoreService(
    EpgSearchNativeFuzzyCapabilityRepository& repository,
    EpgSearchNativeFuzzyCapabilityDetector& detector,
    BackendRegistryService& backendRegistryService)
    : repository_(repository),
      detector_(detector),
      backendRegistryService_(backendRegistryService)
{
}

EpgSearchNativeFuzzyCapabilityRestoreResult
EpgSearchNativeFuzzyCapabilityRestoreService::restoreBackend(
    const std::string& backendId)
{
    EpgSearchNativeFuzzyCapabilityRestoreResult result;

    const auto persistedProbeResult = repository_.load(backendId);

    if (!persistedProbeResult.has_value())
    {
        return result;
    }

    result.persistedResultFound = true;
    result.nativeFuzzyAvailable =
        detector_.nativeFuzzyAvailable(*persistedProbeResult);

    const auto backend = backendRegistryService_.getBackend(backendId);

    if (!backend.has_value())
    {
        return result;
    }

    result.backendFound = true;

    const auto detectedCapabilities = detector_.apply(
        backend->capabilities,
        *persistedProbeResult);

    result.backendUpdated = backendRegistryService_.updateBackendCapabilities(
        backendId,
        detectedCapabilities);

    return result;
}
