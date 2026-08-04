#pragma once

#include "DashboardController.h"
#include "EpgCacheController.h"
#include "GenreBrowserApiRuntime.h"
#include "GlobalSearchApiRuntime.h"
#include "LiveRemoteApiRuntime.h"
#include "ManualRecordingMetadataApiRuntime.h"
#include "SearchTimerPreviewEpgCache.h"
#include "SearchTimerPreviewEpgInputContext.h"
#include "SeriesArtworkSettingsApiRuntime.h"
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
class RecordingActionPreviewController;
class RecordingPersonSearchController;
class RecordingActionValidationController;
class RuntimeDiagnosticsController;
class SearchTimerPreviewEpgCacheRefreshController;
class SnapshotChangeFeedController;
class ISearchTimerCommandExecutor;
class SearchTimerController;
class SearchTimerDiscoveryController;
class SearchTimerAutomationPreviewController;
class VdrController;
class VdrChannelMoveController;
class VdrRecordingQueryController;
class VdrRecordingFolderController;
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
        RecordingActionPreviewController& recordingActionPreviewController,
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
        SearchTimerAutomationPreviewController* searchTimerAutomationPreviewController = nullptr,
        SearchTimerPreviewEpgCacheRefreshController* searchTimerPreviewEpgCacheRefreshController = nullptr,
        IEpgCacheController* epgCacheController = nullptr,
        VdrChannelMoveController* vdrChannelMoveController = nullptr,
        VdrRecordingFolderController* vdrRecordingFolderController = nullptr);

    void setSearchTimerPreviewEpgCache(
        SearchTimerPreviewEpgCache* searchTimerPreviewEpgCache)
    {
        vdrSnapshotReadService_.setSearchTimerPreviewEpgCache(
            searchTimerPreviewEpgCache);
    }

    ApiResponse getEpgArtwork(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const
    {
        if (epgCacheController_ == nullptr)
        {
            ApiResponse response;
            response.statusCode = 503;
            response.contentType = "application/json";
            response.body = "{\"error\":\"epg cache unavailable\"}";
            return response;
        }

        return epgCacheController_->getArtwork(
            backendId,
            channelId,
            eventId);
    }

    ApiResponse handleGet(
        const std::string& path);

    ApiResponse handlePost(
        const std::string& path,
        const std::string& body);

    ApiResponse handleClientGet(
        const std::string& requestTarget)
    {
        ApiResponse response;

        ManualRecordingMetadataApiRuntime::instance().registerController(
            metadataController_);
        if (ManualRecordingMetadataApiRuntime::instance().tryHandleGet(
                requestTarget,
                response))
        {
            return response;
        }

        if (SeriesArtworkSettingsApiRuntime::instance().tryHandleGet(
                requestTarget,
                response))
        {
            return response;
        }

        if (LiveRemoteApiRuntime::instance().tryHandleGet(
                requestTarget,
                response))
        {
            return response;
        }

        if (GlobalSearchApiRuntime::instance().tryHandleGet(
                requestTarget,
                response))
        {
            return response;
        }

        if (GenreBrowserApiRuntime::instance().tryHandleGet(
                requestTarget,
                response))
        {
            return response;
        }

        return handleGet(requestTarget);
    }

    ApiResponse handleClientPost(
        const std::string& requestTarget,
        const std::string& body,
        const std::string& actorRef = "")
    {
        ApiResponse response;

        ManualRecordingMetadataApiRuntime::instance().registerController(
            metadataController_);
        if (ManualRecordingMetadataApiRuntime::instance().tryHandlePost(
                requestTarget,
                body,
                actorRef,
                response))
        {
            return response;
        }

        if (SeriesArtworkSettingsApiRuntime::instance().tryHandlePost(
                requestTarget,
                body,
                response))
        {
            return response;
        }

        if (LiveRemoteApiRuntime::instance().tryHandlePost(
                requestTarget,
                body,
                response))
        {
            return response;
        }

        return handlePost(requestTarget, body);
    }

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
    RecordingActionPreviewController& recordingActionPreviewController_;
    VdrTimerActionController& vdrTimerActionController_;
    VdrTimerActionExecutorAdapterRegistry& vdrTimerActionExecutorAdapterRegistry_;
    RuntimeDiagnosticsController& runtimeDiagnosticsController_;
    SnapshotChangeFeedController& snapshotChangeFeedController_;
    SearchTimerController* searchTimerController_;
    SearchTimerDiscoveryController* searchTimerDiscoveryController_;
    SearchTimerAutomationPreviewController* searchTimerAutomationPreviewController_;
    SearchTimerPreviewEpgCacheRefreshController* searchTimerPreviewEpgCacheRefreshController_;
    IEpgCacheController* epgCacheController_;
    LiveTransportController& liveTransportController_;
    ISearchTimerCommandExecutor* searchTimerCommandExecutor_;
    EpgSearchNativeFuzzyStaleProbeAdministrationController* nativeFuzzyStaleProbeAdministrationController_;
    EpgSearchNativeFuzzyOperatorRefreshController* nativeFuzzyOperatorRefreshController_;
    VdrChannelMoveController* vdrChannelMoveController_;
    VdrRecordingFolderController* vdrRecordingFolderController_;
};
