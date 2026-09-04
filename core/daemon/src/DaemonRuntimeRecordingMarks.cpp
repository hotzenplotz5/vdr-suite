#include "DaemonRuntimeRecordingMarks.h"

#include "RecordingMarksApiRuntime.h"

#include <string>

bool configureDaemonRecordingMarksRuntime(
    VdrRecordingCacheRepository& recordingCacheRepository,
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts)
{
    VdrRecordingCacheRepository* const recordingCache =
        &recordingCacheRepository;
    const auto* const runtimeContexts = &backendRuntimeContexts;

    return RecordingMarksApiRuntime::instance().configure(
        [recordingCache](const std::string& backendId) {
            return recordingCache->findAllForBackend(backendId);
        },
        [runtimeContexts](const std::string& backendId) {
            for (const auto& backendRuntimeContext : *runtimeContexts) {
                if (!backendRuntimeContext ||
                    backendRuntimeContext->backendId != backendId)
                {
                    continue;
                }

                if (!backendRuntimeContext->suiteBridgeAgentRuntime) {
                    return RecordingMarksBackendAccess{
                        RecordingMarksBackendAvailability::CapabilityUnavailable,
                        nullptr};
                }

                const auto health =
                    backendRuntimeContext->suiteBridgeAgentRuntime->health();
                if (!health.running ||
                    !health.observation.hasDiscovery ||
                    !health.observation.discovery.capabilityAvailable(
                        "recording-marks"))
                {
                    return RecordingMarksBackendAccess{
                        RecordingMarksBackendAvailability::CapabilityUnavailable,
                        nullptr};
                }

                SuiteBridgeRecordingMarksResolver* resolver =
                    backendRuntimeContext->ensureRecordingMarksResolver();
                if (resolver == nullptr) {
                    return RecordingMarksBackendAccess{
                        RecordingMarksBackendAvailability::CapabilityUnavailable,
                        nullptr};
                }

                return RecordingMarksBackendAccess{
                    RecordingMarksBackendAvailability::Available,
                    resolver};
            }

            return RecordingMarksBackendAccess{
                RecordingMarksBackendAvailability::BackendNotFound,
                nullptr};
        });
}

void resetDaemonRecordingMarksRuntime()
{
    RecordingMarksApiRuntime::instance().reset();
}
