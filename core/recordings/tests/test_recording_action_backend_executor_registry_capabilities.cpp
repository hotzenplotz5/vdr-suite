#include "MockHttpClient.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"

#include <cassert>
#include <memory>

namespace
{
RestfulApiRecordingActionBackendConfig makeConfig(
    const std::string& backendId)
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = backendId;
    config.basePath = "/api";
    config.readOnly = false;
    config.allowExecution = false;
    return config;
}
}

int main()
{
    MockHttpClient httpClient;
    RecordingActionBackendExecutorAdapterRegistry registry;

    registry.registerAdapter(
        std::make_shared<RestfulApiRecordingActionBackendExecutorAdapter>(
            makeConfig("living-room"),
            httpClient));

    {
        const RecordingActionCapabilitySet capabilities =
            registry.capabilitiesForBackend("living-room");

        assert(capabilities.contains("recording.action.move"));
        assert(capabilities.contains("recording.action.rename"));
        assert(capabilities.contains("recording.action.delete"));
        assert(!capabilities.contains("recording.action.cut"));
    }

    assert(registry.backendSupportsAction(
        "living-room",
        RecordingActionType::Move));

    assert(registry.backendSupportsAction(
        "living-room",
        RecordingActionType::Rename));

    assert(registry.backendSupportsAction(
        "living-room",
        RecordingActionType::Delete));

    assert(!registry.backendSupportsAction(
        "living-room",
        RecordingActionType::Cut));

    assert(!registry.backendSupportsAction(
        "missing-backend",
        RecordingActionType::Delete));

    const RecordingActionCapabilitySet missingCapabilities =
        registry.capabilitiesForBackend("missing-backend");

    assert(missingCapabilities.capabilities.empty());

    return 0;
}
