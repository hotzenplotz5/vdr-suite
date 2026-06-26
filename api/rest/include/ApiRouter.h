#pragma once

#include "DashboardController.h"
#include "SearchTimerPreviewEpgCache.h"
#include "SearchTimerPreviewEpgInputContext.h"
#include "VdrSnapshotReadService.h"

#include <string>
#include <vector>

class BackendRegistryController;
class CapabilityController;
class EpgController;
class EpgSearchNativeFuzzyStaleProbeAdministrationController;
class EpgSearchNativeFuzzyOperatorRefreshController;
class JobsController;
class LiveTransportController;
class MetadataController;
class PersonController;
class RecordingsController;
class RecordingActionExecutionController;
class RecordingPersonSearchController;
class RecordingActionValidationController;
class RuntimeDiagnosticsController;
class SnapshotChangeFeedController;
class ISearchTimerCommandExecutor;
class SearchTimerController;
class SearchTimerDiscoveryController;
class SearchTimerAutomationPreviewController;
class VdrController;
class VdrRecordingQueryController;
class VdrTimerActionController;
class VdrTimerActionExecutorAdapterRegistry;

class SearchTimerPreviewSnapshotReadFacade
{
public:
    explicit SearchTimerPreviewSnapshotReadFacade(
        VdrSnapshotReadService& snapshotReadService)
        : snapshotReadService_(snapshotReadService),
          searchTimerPreviewEpgCache_(nullptr)
    {
    }

    void setSearchTimerPreviewEpgCache(
        SearchTimerPreviewEpgCache* searchTimerPreviewEpgCache)
    {
        searchTimerPreviewEpgCache_ = searchTimerPreviewEpgCache;
    }

    std::vector<VdrRecording> getRecordings() const
    {
        return snapshotReadService_.getRecordings();
    }

    std::vector<VdrRecording> getRecordingsForBackend(
        const std::string& backendId) const
    {
        return snapshotReadService_.getRecordingsForBackend(backendId);
    }

    std::vector<VdrEvent> getEvents() const
    {
        return getEventsForBackend("default");
    }

    std::vector<VdrEvent> getEventsForBackend(
        const std::string& backendId) const
    {
        if (searchTimerPreviewEpgCache_ == nullptr)
        {
            SearchTimerPreviewEpgInputContext::resetReady();
            return snapshotReadService_.getEventsForBackend(backendId);
        }

        const SearchTimerPreviewEpgCacheStatus status =
            searchTimerPreviewEpgCache_->statusForBackend(backendId);

        SearchTimerPreviewEpgInputContext::setCacheStatus(
            status,
            backendId);

        const std::vector<VdrEvent>* cachedEvents =
            searchTimerPreviewEpgCache_->eventsForBackend(backendId);

        if (cachedEvents != nullptr)
        {
            return *cachedEvents;
        }

        return snapshotReadService_.getEventsForBackend(backendId);
    }

private:
    VdrSnapshotReadService& snapshotReadService_;
    SearchTimerPreviewEpgCache* searchTimerPreviewEpgCache_;
};

class ApiRouter
{
public:
    ApiRouter(
        DashboardController& dashboardController,
        JobsController& jobsController,
        RecordingsController& recordingsController,
        MetadataController& metadataController,
        VdrController& vdrController,
        VdrRecordingQueryController& vdrRecordingQueryController,
        VdrSnapshotReadService& vdrSnapshotReadService,
        EpgController* epgController,
        PersonController* personController,
        RecordingPersonSearchController* recordingPersonSearchController,
        BackendRegistryController& backendRegistryController,
        CapabilityController& capabilityController,
        RecordingActionValidationController& recordingActionValidationController,
        RecordingActionExecutionController& recordingActionExecutionController,
        VdrTimerActionController& vdrTimerActionController,
        VdrTimerActionExecutorAdapterRegistry& vdrTimerActionExecutorAdapterRegistry,
        RuntimeDiagnosticsController& runtimeDiagnosticsController,
        SnapshotChangeFeedController& snapshotChangeFeedController,
        SearchTimerController* searchTimerController,
        LiveTransportController& liveTransportController,
        ISearchTimerCommandExecutor* searchTimerCommandExecutor = nullptr,
        EpgSearchNativeFuzzyStaleProbeAdministrationController* nativeFuzzyStaleProbeAdministrationController = nullptr,
        EpgSearchNativeFuzzyOperatorRefreshController* nativeFuzzyOperatorRefreshController = nullptr,
        SearchTimerDiscoveryController* searchTimerDiscoveryController = nullptr,
        SearchTimerAutomationPreviewController* searchTimerAutomationPreviewController = nullptr);

    void setSearchTimerPreviewEpgCache(
        SearchTimerPreviewEpgCache* searchTimerPreviewEpgCache)
    {
        vdrSnapshotReadService_.setSearchTimerPreviewEpgCache(
            searchTimerPreviewEpgCache);
    }

    ApiResponse handleGet(
        const std::string& path);

    ApiResponse handlePost(
        const std::string& path,
        const std::string& body);

private:
    DashboardController& dashboardController_;
    JobsController& jobsController_;
    RecordingsController& recordingsController_;
    MetadataController& metadataController_;
    VdrController& vdrController_;
    VdrRecordingQueryController& vdrRecordingQueryController_;
    SearchTimerPreviewSnapshotReadFacade vdrSnapshotReadService_;
    EpgController* epgController_;
    PersonController* personController_;
    RecordingPersonSearchController* recordingPersonSearchController_;
    BackendRegistryController& backendRegistryController_;
    CapabilityController& capabilityController_;
    RecordingActionValidationController& recordingActionValidationController_;
    RecordingActionExecutionController& recordingActionExecutionController_;
    VdrTimerActionController& vdrTimerActionController_;
    VdrTimerActionExecutorAdapterRegistry& vdrTimerActionExecutorAdapterRegistry_;
    RuntimeDiagnosticsController& runtimeDiagnosticsController_;
    SnapshotChangeFeedController& snapshotChangeFeedController_;
    SearchTimerController* searchTimerController_;
    SearchTimerDiscoveryController* searchTimerDiscoveryController_;
    SearchTimerAutomationPreviewController* searchTimerAutomationPreviewController_;
    LiveTransportController& liveTransportController_;
    ISearchTimerCommandExecutor* searchTimerCommandExecutor_;
    EpgSearchNativeFuzzyStaleProbeAdministrationController* nativeFuzzyStaleProbeAdministrationController_;
    EpgSearchNativeFuzzyOperatorRefreshController* nativeFuzzyOperatorRefreshController_;
};
