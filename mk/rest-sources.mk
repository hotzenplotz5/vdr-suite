REST_DASHBOARD_SRC := \
        core/recordings/src/JobRepository.cpp \
        core/recordings/src/JobDashboardService.cpp \
        core/recordings/src/RecordingRepository.cpp \
        core/recordings/src/MetadataRepository.cpp \
        core/recordings/src/RecordingDashboardService.cpp \
        core/recordings/src/DashboardFacade.cpp \
        core/recordings/src/DashboardJsonSerializer.cpp \
        api/rest/src/DashboardController.cpp

REST_VDR_SRC := \
        api/rest/src/VdrController.cpp \
        api/rest/src/VdrRecordingQueryController.cpp

REST_EPG_SRC := \
        api/rest/src/EpgController.cpp \
        api/rest/src/RestQueryParameters.cpp

REST_RUNTIME_SRC := \
        api/rest/src/RuntimeDiagnosticsController.cpp

REST_SNAPSHOT_CHANGE_FEED_SRC := \
        api/rest/src/SnapshotChangeFeedController.cpp

REST_LIVE_TRANSPORT_SRC := \
        api/rest/src/LiveTransportController.cpp

VDR_TIMER_ACTION_REST_PARSER_SRC := \
        api/rest/src/VdrTimerActionRequestParser.cpp

REST_LIVE_REMOTE_SRC := \
        api/rest/src/RemoteActionRequestParser.cpp \
        api/rest/src/RemoteActionController.cpp \
        api/rest/src/LiveOverlayController.cpp \
        api/rest/src/LiveRemoteApiRuntime.cpp

REST_MANUAL_RECORDING_METADATA_SRC := \
        $(METADATA_PLATFORM_SRC) \
        $(METADATA_GENRE_SRC) \
        $(MANUAL_RECORDING_METADATA_SRC) \
        core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp \
        core/http/src/CurlExternalArtworkHttpTransport.cpp \
        api/rest/src/ManualRecordingMetadataApiRuntime.cpp

REST_ROUTER_SRC := \
        $(GLOBAL_SEARCH_SRC) \
        $(GENRE_BROWSER_REST_SRC) \
        $(REST_LIVE_REMOTE_SRC) \
        $(REST_MANUAL_RECORDING_METADATA_SRC) \
        core/recordings/src/JobRepository.cpp \
        core/recordings/src/JobDashboardService.cpp \
        core/recordings/src/RecordingRepository.cpp \
        core/recordings/src/MetadataRepository.cpp \
        core/recordings/src/RecordingDashboardService.cpp \
        core/recordings/src/DashboardFacade.cpp \
        core/recordings/src/DashboardJsonSerializer.cpp \
        api/rest/src/DashboardController.cpp \
        api/rest/src/JobsController.cpp \
        api/rest/src/RecordingsController.cpp \
        api/rest/src/MetadataController.cpp \
        api/rest/src/PersonController.cpp \
        api/rest/src/RecordingPersonSearchController.cpp \
        core/vdr/src/RecordingPersonSearchService.cpp \
        core/vdr/src/RecordingPersonSearchResultJsonSerializer.cpp \
        core/vdr/src/PersonQueryMatcher.cpp \
        core/vdr/src/PersonQueryResultJsonSerializer.cpp \
        core/vdr/src/PersonResolutionJsonSerializer.cpp \
        core/vdr/src/PersonSearchService.cpp \
        api/rest/src/EpgController.cpp \
        api/rest/src/EpgArtworkController.cpp \
        api/rest/src/RestQueryParameters.cpp \
        api/rest/src/BackendRegistryController.cpp \
        api/rest/src/RuntimeDiagnosticsController.cpp \
        api/rest/src/SnapshotChangeFeedController.cpp \
        api/rest/src/LiveTransportController.cpp \
        api/rest/src/CapabilityController.cpp \
        api/rest/src/RecordingActionValidationController.cpp \
        api/rest/src/RecordingActionExecutionController.cpp \
        api/rest/src/RecordingActionPreviewController.cpp \
        api/rest/src/VdrTimerActionController.cpp \
        api/rest/src/VdrChannelMoveRequestParser.cpp \
        api/rest/src/VdrChannelMoveController.cpp \
        api/rest/src/SearchTimerPreviewEpgCacheRefreshController.cpp \
        core/recordings/src/RecordingActionUtils.cpp \
        core/recordings/src/RecordingActionValidationResultJsonSerializer.cpp \
        core/recordings/src/RecordingActionExecutionResultJsonSerializer.cpp \
        core/recordings/src/RecordingActionRequestPreviewResultJsonSerializer.cpp \
        core/recordings/src/RecordingActionSafetyResultJsonSerializer.cpp \
        core/recordings/src/RecordingActionValidationService.cpp \
        api/rest/src/RecordingActionValidationRequestParser.cpp \
        core/vdr/src/VdrTimerActionService.cpp \
        core/vdr/src/VdrTimerActionExecutionService.cpp \
        core/vdr/src/VdrTimerActionResultJsonSerializer.cpp \
        core/vdr/src/MockVdrTimerActionExecutor.cpp \
        api/rest/src/VdrTimerActionRequestParser.cpp \
        api/rest/src/SearchTimerAutomationPreviewController.cpp \
        api/rest/src/ApiRouter.cpp
