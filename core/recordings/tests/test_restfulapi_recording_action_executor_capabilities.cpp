#include "MockHttpClient.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"

#include <cassert>

namespace
{
RestfulApiRecordingActionBackendConfig makeConfig()
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "default";
    config.basePath = "/api";
    config.readOnly = false;
    config.allowExecution = false;
    return config;
}
}

int main()
{
    MockHttpClient httpClient;

    RestfulApiRecordingActionBackendExecutorAdapter adapter(
        makeConfig(),
        httpClient);

    assert(adapter.backendId() == "default");
    assert(adapter.backendType() == "restfulapi");

    const RecordingActionCapabilitySet capabilities =
        adapter.capabilities();

    assert(capabilities.contains("recording.action.move"));
    assert(capabilities.contains("recording.action.rename"));
    assert(capabilities.contains("recording.action.delete"));

    assert(!capabilities.contains("recording.action.cut"));
    assert(!capabilities.contains("recording.action.repair"));
    assert(!capabilities.contains("recording.action.shrink"));
    assert(!capabilities.contains("recording.action.metadata-refresh"));

    assert(adapter.supportsAction(RecordingActionType::Move));
    assert(adapter.supportsAction(RecordingActionType::Rename));
    assert(adapter.supportsAction(RecordingActionType::Delete));

    assert(!adapter.supportsAction(RecordingActionType::Cut));
    assert(!adapter.supportsAction(RecordingActionType::Repair));
    assert(!adapter.supportsAction(RecordingActionType::Shrink));
    assert(!adapter.supportsAction(RecordingActionType::MetadataRefresh));
    assert(!adapter.supportsAction(RecordingActionType::Unknown));

    return 0;
}
