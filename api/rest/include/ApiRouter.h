#pragma once

#include "DashboardController.h"

#include <string>

class BackendRegistryController;
class CapabilityController;
class EpgController;
class EpgSearchNativeFuzzyStaleProbeAdministrationController;
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
class VdrController;
class VdrRecordingQueryController;
class VdrSnapshotReadService;
class VdrTimerActionController;
class VdrTimerActionExecutorAdapterRegistry;

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
        EpgSearchNativeFuzzyStaleProbeAdministrationController* nativeFuzzyStaleProbeAdministrationController = nullptr);

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
    VdrSnapshotReadService& vdrSnapshotReadService_;
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
    LiveTransportController& liveTransportController_;
    ISearchTimerCommandExecutor* searchTimerCommandExecutor_;
    EpgSearchNativeFuzzyStaleProbeAdministrationController* nativeFuzzyStaleProbeAdministrationController_;
};
