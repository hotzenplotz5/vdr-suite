#include "EpgSearchNativeFuzzyOperatorRefreshService.h"

#include "BackendRegistryService.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "ISearchTimerCommandExecutor.h"
#include "ISearchTimerDataSource.h"
#include "SearchTimerCreateRequest.h"
#include "SearchTimerDeleteRequest.h"
#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"

#include <string>

namespace
{
    std::string normalizedBackendId(const std::string& backendId)
    {
        return backendId.empty() ? "default" : backendId;
    }

    std::string normalizedProbeQuery(const std::string& probeQuery)
    {
        return probeQuery.empty()
            ? "VDR-Suite Native Fuzzy Operator Refresh"
            : probeQuery;
    }

    int normalizedTolerance(int tolerance)
    {
        return tolerance <= 0 ? 2 : tolerance;
    }

    SearchTimerCreateRequest buildProbeCreateRequest(
        const std::string& backendId,
        const std::string& probeQuery,
        int tolerance)
    {
        SearchTimerCreateRequest request;
        request.backendId = backendId;
        request.name = probeQuery;
        request.query = probeQuery;
        request.active = true;
        request.directory = "VDR-Suite-Validation";
        request.compareTitle = true;
        request.compareSubtitle = true;
        request.compareSummary = true;
        request.matchMode = 5;
        request.matchTolerance = tolerance;
        return request;
    }

    const SearchTimer* findReadbackTimer(
        const SearchTimerResult& searchTimerResult,
        const std::string& backendNativeId,
        const std::string& probeQuery)
    {
        for (const auto& timer : searchTimerResult.items())
        {
            if (!backendNativeId.empty()
                && timer.backendNativeId() == backendNativeId)
            {
                return &timer;
            }
        }

        for (const auto& timer : searchTimerResult.items())
        {
            if (timer.query() == probeQuery)
            {
                return &timer;
            }
        }

        return nullptr;
    }

    std::string statusFromProbe(
        bool createAccepted,
        bool readbackAvailable,
        bool modePreserved,
        bool tolerancePreserved,
        bool cleanupSucceeded,
        bool nativeFuzzyAvailable)
    {
        if (!createAccepted)
        {
            return "create-failed";
        }

        if (!readbackAvailable)
        {
            return "readback-unavailable";
        }

        if (!modePreserved)
        {
            return "mode-not-preserved";
        }

        if (!tolerancePreserved)
        {
            return "tolerance-not-preserved";
        }

        if (!cleanupSucceeded)
        {
            return "cleanup-failed";
        }

        return nativeFuzzyAvailable
            ? "refreshed-native-available"
            : "refreshed-native-unavailable";
    }
}

EpgSearchNativeFuzzyOperatorRefreshService::EpgSearchNativeFuzzyOperatorRefreshService(
    ISearchTimerCommandExecutor& commandExecutor,
    ISearchTimerDataSource& dataSource,
    EpgSearchNativeFuzzyCapabilityRepository& repository,
    EpgSearchNativeFuzzyCapabilityDetector& detector,
    BackendRegistryService& backendRegistryService)
    : commandExecutor_(commandExecutor),
      dataSource_(dataSource),
      repository_(repository),
      detector_(detector),
      backendRegistryService_(backendRegistryService)
{
}

EpgSearchNativeFuzzyOperatorRefreshSummary
EpgSearchNativeFuzzyOperatorRefreshService::refresh(
    const EpgSearchNativeFuzzyOperatorRefreshRequest& request)
{
    EpgSearchNativeFuzzyOperatorRefreshSummary summary;
    summary.backendId = normalizedBackendId(request.backendId);
    summary.probeQuery = normalizedProbeQuery(request.probeQuery);
    summary.tolerance = normalizedTolerance(request.tolerance);

    const auto backend =
        backendRegistryService_.getBackend(summary.backendId);

    if (!backend.has_value())
    {
        summary.status = "backend-not-found";
        summary.errors.push_back("backend not found: " + summary.backendId);
        return summary;
    }

    summary.backendKnown = true;
    summary.createAttempted = true;

    const auto createResult =
        commandExecutor_.create(
            buildProbeCreateRequest(
                summary.backendId,
                summary.probeQuery,
                summary.tolerance));

    summary.createAccepted = createResult.success;

    if (!createResult.success)
    {
        summary.status = "create-failed";
        summary.errors = createResult.errors;
        return summary;
    }

    summary.backendNativeId =
        createResult.searchTimer.backendNativeId();

    const auto searchTimerResult =
        dataSource_.list(
            SearchTimerQuery::byBackend(summary.backendId)
                .withText(summary.probeQuery));

    const SearchTimer* readbackTimer =
        findReadbackTimer(
            searchTimerResult,
            summary.backendNativeId,
            summary.probeQuery);

    summary.readbackAvailable = readbackTimer != nullptr;

    if (readbackTimer != nullptr)
    {
        summary.modePreserved =
            readbackTimer->matchOptions().mode() == 5;
        summary.tolerancePreserved =
            readbackTimer->matchOptions().tolerance() == summary.tolerance;
    }

    if (!request.keepProbeSearchTimer)
    {
        summary.cleanupAttempted = true;

        SearchTimerDeleteRequest deleteRequest;
        deleteRequest.backendId = summary.backendId;
        deleteRequest.backendNativeId = summary.backendNativeId;

        const auto deleteResult =
            commandExecutor_.remove(deleteRequest);

        summary.cleanupSucceeded = deleteResult.success;

        if (!deleteResult.success)
        {
            summary.errors.insert(
                summary.errors.end(),
                deleteResult.errors.begin(),
                deleteResult.errors.end());
        }
    }
    else
    {
        summary.warnings.push_back(
            "temporary probe SearchTimer was kept by operator request");
    }

    EpgSearchNativeFuzzyCapabilityProbeResult probeResult;
    probeResult.createAccepted = summary.createAccepted;
    probeResult.readbackAvailable = summary.readbackAvailable;
    probeResult.modePreserved = summary.modePreserved;
    probeResult.tolerancePreserved = summary.tolerancePreserved;
    probeResult.cleanupSucceeded = summary.cleanupSucceeded;

    summary.nativeFuzzyAvailable =
        detector_.nativeFuzzyAvailable(probeResult);

    if (repository_.ensureSchema())
    {
        summary.persisted =
            repository_.save(
                summary.backendId,
                probeResult);
    }

    if (request.updateBackendCapabilities)
    {
        const auto refreshedCapabilities =
            detector_.apply(
                backend->capabilities,
                probeResult);

        summary.backendCapabilitiesUpdated =
            backendRegistryService_.updateBackendCapabilities(
                summary.backendId,
                refreshedCapabilities);
    }

    summary.status = statusFromProbe(
        summary.createAccepted,
        summary.readbackAvailable,
        summary.modePreserved,
        summary.tolerancePreserved,
        summary.cleanupSucceeded,
        summary.nativeFuzzyAvailable);

    return summary;
}
