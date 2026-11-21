#include "DaemonRuntimeRecordingEditing.h"

#include "DaemonRuntimeRecordingCut.h"
#include "DaemonRuntimeRecordingMarks.h"

bool configureDaemonRecordingEditingRuntime(
    VdrRecordingCacheRepository& recordingCacheRepository,
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts,
    BackendRegistryService& backendRegistryService,
    BackendAccessPolicy& backendAccessPolicy,
    BackendAgentRepository& backendAgentRepository,
    BackendAgentCommandRepository& backendAgentCommandRepository)
{
    if (!configureDaemonRecordingMarksRuntime(
            recordingCacheRepository,
            backendRuntimeContexts,
            backendRegistryService,
            backendAccessPolicy,
            backendAgentRepository,
            backendAgentCommandRepository))
        return false;

    if (!configureDaemonRecordingCutRuntime(
            recordingCacheRepository,
            backendRuntimeContexts,
            backendRegistryService,
            backendAccessPolicy,
            backendAgentRepository,
            backendAgentCommandRepository))
    {
        resetDaemonRecordingMarksRuntime();
        return false;
    }
    return true;
}

void resetDaemonRecordingEditingRuntime()
{
    resetDaemonRecordingCutRuntime();
    resetDaemonRecordingMarksRuntime();
}
