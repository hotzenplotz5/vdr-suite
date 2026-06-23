#pragma once

#include "ApiRouter.h"
#include "BackendRuntimeContext.h"
#include "BackendPollingCoordinator.h"
#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryController.h"
#include "VdrCapabilitySet.h"
#include "CapabilityResolver.h"
#include "CapabilityReportService.h"
#include "CapabilityReportJsonSerializer.h"
#include "CapabilityReportBuilder.h"
#include "CapabilityController.h"
#include "DashboardController.h"
#include "DashboardFacade.h"
#include "DashboardJsonSerializer.h"
#include "EpgController.h"
#include "EpgQueryService.h"
#include "EpgSearchResultJsonSerializer.h"
#include "EpgSearchService.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "EpgSearchNativeFuzzyStartupRestoreService.h"
#include "ConsoleRuntimeLogger.h"
#include "Database.h"
#include "IHttpClient.h"
#include "IVdrAdapter.h"
#include "IHttpServer.h"
#include "JobDashboardService.h"
#include "JobRepository.h"
#include "JobsController.h"
#include "LiveTransportController.h"
#include "LiveTransportService.h"
#include "MetadataController.h"
#include "MetadataRepository.h"
#include "PollingService.h"
#include "PersonSearchService.h"
#include "PersonResolutionJsonSerializer.h"
#include "PersonQueryResultJsonSerializer.h"
#include "PersonController.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionExecutionController.h"
#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationController.h"
#include "RecordingActionValidationRequestParser.h"
#include "RecordingActionValidationResultJsonSerializer.h"
#include "RecordingActionValidationService.h"
#include "RecordingsController.h"
#include "RecordingPersonSearchService.h"
#include "RecordingPersonSearchResultJsonSerializer.h"
#include "RecordingPersonSearchController.h"
#include "RuntimeConfig.h"
#include "RuntimeDiagnosticsController.h"
#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeDiagnosticsService.h"
#include "SearchTimerController.h"
#include "RestfulApiSearchTimerCommandExecutor.h"
#include "SearchTimerResultJsonSerializer.h"
#include "SearchTimerService.h"
#include "SnapshotAccessService.h"
#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedController.h"
#include "SnapshotChangeFeedJsonSerializer.h"
#include "SnapshotChangeFeedService.h"
#include "SseLiveTransport.h"
#include "VdrSnapshotReadJsonSerializer.h"
#include "VdrSnapshotReadService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrConfig.h"
#include "VdrController.h"
#include "VdrRecordingQueryController.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryService.h"
#include "VdrTimerActionController.h"
#include "VdrTimerActionExecutionService.h"
#include "VdrTimerActionExecutorAdapterRegistry.h"
#include "VdrTimerActionRequestParser.h"
#include "VdrTimerActionResultJsonSerializer.h"
#include "VdrTimerActionService.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"
#include "SimpleHttpListener.h"

#include <atomic>
#include <memory>
#include <vector>

class DaemonRuntime
{
public:
    DaemonRuntime();

    bool initialize();
    int run();
    void shutdown();

private:
    static void handleSignal(int signalNumber);
    void pollVdrAndUpdateChangeFeed();
    std::unique_ptr<BackendRuntimeContext> createBackendRuntimeContext(
        const BackendNode& backend);

    bool initialized_;

    RuntimeConfig config_;
    ConsoleRuntimeLogger runtimeLogger_;
    RuntimeDiagnosticsService runtimeDiagnosticsService_;
    Database database_;

    std::unique_ptr<JobRepository> jobRepository_;
    std::unique_ptr<RecordingRepository> recordingRepository_;
    std::unique_ptr<MetadataRepository> metadataRepository_;

    std::unique_ptr<JobDashboardService> jobDashboardService_;
    std::unique_ptr<RecordingDashboardService> recordingDashboardService_;
    std::unique_ptr<DashboardFacade> dashboardFacade_;

    std::unique_ptr<DashboardJsonSerializer> dashboardJsonSerializer_;
    std::unique_ptr<DashboardController> dashboardController_;
    std::unique_ptr<JobsController> jobsController_;
    std::unique_ptr<RecordingsController> recordingsController_;
    std::unique_ptr<MetadataController> metadataController_;

    BackendRegistry backendRegistry_;
    std::unique_ptr<BackendRegistryService> backendRegistryService_;
    std::unique_ptr<BackendRegistryJsonSerializer> backendRegistryJsonSerializer_;
    std::unique_ptr<BackendRegistryController> backendRegistryController_;
    VdrConfig vdrConfig_;
    std::vector<std::unique_ptr<BackendRuntimeContext>> backendRuntimeContexts_;
    std::unique_ptr<BackendPollingCoordinator> backendPollingCoordinator_;
    std::unique_ptr<SnapshotCache> snapshotCache_;
    std::unique_ptr<SnapshotCacheService> snapshotCacheService_;
    std::unique_ptr<SnapshotAccessService> snapshotAccessService_;
    std::unique_ptr<VdrSnapshotReadService> vdrSnapshotReadService_;
    std::unique_ptr<VdrSnapshotReadJsonSerializer> vdrSnapshotReadJsonSerializer_;
    std::unique_ptr<SnapshotChangeFeed> snapshotChangeFeed_;
    std::unique_ptr<SnapshotChangeFeedService> snapshotChangeFeedService_;
    std::unique_ptr<SnapshotChangeFeedJsonSerializer> snapshotChangeFeedJsonSerializer_;
    std::unique_ptr<SnapshotChangeFeedController> snapshotChangeFeedController_;
    std::unique_ptr<SseLiveTransport> liveTransport_;
    std::unique_ptr<LiveTransportService> liveTransportService_;
    std::unique_ptr<LiveTransportController> liveTransportController_;
    std::unique_ptr<VdrOverviewService> vdrOverviewService_;
    std::unique_ptr<VdrOverviewJsonSerializer> vdrOverviewJsonSerializer_;
    std::unique_ptr<VdrController> vdrController_;
    std::unique_ptr<VdrRecordingQueryService> vdrRecordingQueryService_;
    std::unique_ptr<VdrRecordingQueryResultJsonSerializer> vdrRecordingQueryResultJsonSerializer_;
    std::unique_ptr<VdrRecordingQueryController> vdrRecordingQueryController_;
    std::unique_ptr<VdrCapabilitySet> capabilitySet_;
    std::unique_ptr<CapabilityResolver> capabilityResolver_;
    std::unique_ptr<CapabilityReportBuilder> capabilityReportBuilder_;
    std::unique_ptr<CapabilityReportService> capabilityReportService_;
    std::unique_ptr<CapabilityReportJsonSerializer> capabilityReportJsonSerializer_;
    std::unique_ptr<CapabilityController> capabilityController_;
    std::unique_ptr<EpgSearchNativeFuzzyCapabilityRepository> epgSearchNativeFuzzyCapabilityRepository_;
    std::unique_ptr<EpgSearchNativeFuzzyCapabilityDetector> epgSearchNativeFuzzyCapabilityDetector_;
    std::unique_ptr<EpgSearchNativeFuzzyStartupRestoreService> epgSearchNativeFuzzyStartupRestoreService_;
    std::unique_ptr<EpgQueryService> epgQueryService_;
    std::unique_ptr<EpgSearchService> epgSearchService_;
    std::unique_ptr<EpgSearchResultJsonSerializer> epgSearchResultJsonSerializer_;
    std::unique_ptr<EpgController> epgController_;
    std::unique_ptr<SearchTimerService> searchTimerService_;
    std::unique_ptr<SearchTimerResultJsonSerializer> searchTimerResultJsonSerializer_;
    std::unique_ptr<SearchTimerController> searchTimerController_;
    std::unique_ptr<RestfulApiSearchTimerCommandExecutor> searchTimerCommandExecutor_;
    std::unique_ptr<PersonResolutionJsonSerializer> personResolutionJsonSerializer_;
    std::unique_ptr<PersonSearchService> personSearchService_;
    std::unique_ptr<PersonQueryResultJsonSerializer> personQueryResultJsonSerializer_;
    std::unique_ptr<PersonController> personController_;
    std::unique_ptr<RecordingPersonSearchService> recordingPersonSearchService_;
    std::unique_ptr<RecordingPersonSearchResultJsonSerializer> recordingPersonSearchResultJsonSerializer_;
    std::unique_ptr<RecordingPersonSearchController> recordingPersonSearchController_;

    std::unique_ptr<RecordingActionValidationService> recordingActionValidationService_;
    std::unique_ptr<RecordingActionValidationResultJsonSerializer> recordingActionValidationResultJsonSerializer_;
    std::unique_ptr<RecordingActionValidationRequestParser> recordingActionValidationRequestParser_;
    std::unique_ptr<RecordingActionValidationController> recordingActionValidationController_;
    std::unique_ptr<RecordingActionExecutionService> recordingActionExecutionService_;
    std::unique_ptr<RecordingActionExecutionResultJsonSerializer> recordingActionExecutionResultJsonSerializer_;
    std::unique_ptr<RecordingActionBackendExecutorAdapterRegistry> recordingActionBackendExecutorAdapterRegistry_;
    std::unique_ptr<RecordingActionExecutionController> recordingActionExecutionController_;

    std::unique_ptr<VdrTimerActionService> vdrTimerActionService_;
    std::unique_ptr<VdrTimerActionExecutionService> vdrTimerActionExecutionService_;
    std::unique_ptr<VdrTimerActionResultJsonSerializer> vdrTimerActionResultJsonSerializer_;
    std::unique_ptr<VdrTimerActionRequestParser> vdrTimerActionRequestParser_;
    std::unique_ptr<VdrTimerActionExecutorAdapterRegistry> vdrTimerActionExecutorAdapterRegistry_;
    std::unique_ptr<VdrTimerActionController> vdrTimerActionController_;

    std::unique_ptr<RuntimeDiagnosticsJsonSerializer> runtimeDiagnosticsJsonSerializer_;
    std::unique_ptr<RuntimeDiagnosticsController> runtimeDiagnosticsController_;

    std::unique_ptr<ApiRouter> apiRouter_;
    std::unique_ptr<IHttpServer> httpServer_;
    std::unique_ptr<SimpleHttpListener> httpListener_;

    static std::atomic<bool> shutdownRequested_;
};
