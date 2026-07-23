#include "BackendRegistry.h"
#include "LiveRemoteApiRuntime.h"
#include "MockHttpClient.h"
#include "SnapshotAccessService.h"
#include "SnapshotCache.h"

#include <cassert>

int main()
{
    BackendNode node;
    node.backendId = "default";
    node.backendName = "Default";
    node.accessMode = "read-write";
    node.enabled = true;
    node.capabilities.remoteControl = true;
    node.capabilities.liveOverlayRead = true;

    BackendRegistry registry;
    registry.addBackend(node);
    BackendRegistryService registryService(registry);

    VdrSnapshot snapshot;
    snapshot.backendId = "default";
    VdrChannel channel;
    channel.id = "C-1";
    channel.number = 1;
    channel.name = "Das Erste HD";
    snapshot.channels.push_back(channel);

    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    cacheService.updateSnapshotForBackend("default", snapshot);
    SnapshotAccessService accessService(cacheService);
    VdrSnapshotReadService readService(accessService);

    MockHttpClient httpClient;
    HttpResponse response;
    response.statusCode = 200;
    response.body = "{\"channel\":\"C-1\"}";
    httpClient.setResponse(response);

    int published = 0;
    LiveRemoteApiRuntime& runtime = LiveRemoteApiRuntime::instance();
    runtime.configure(
        registryService,
        readService,
        cacheService,
        [&published](const std::string& backendId)
        {
            assert(backendId == "default");
            ++published;
        });
    runtime.registerRestfulApiBackend("default", httpClient);

    ApiResponse apiResponse;
    bool handled = runtime.tryHandlePost(
        "/api/vdr/remote/actions",
        "{\"backendId\":\"default\",\"operationId\":\"route-1\",\"action\":\"ok\"}",
        apiResponse);
    assert(handled);
    assert(apiResponse.statusCode == 200);
    assert(httpClient.lastRequest().url == "/remote/ok");
    assert(published == 1);

    response.statusCode = 200;
    response.body = "{\"channel\":\"C-1\"}";
    httpClient.setResponse(response);
    handled = runtime.tryHandleGet(
        "/api/vdr/live/overlay?backend=default",
        apiResponse);
    assert(handled);
    assert(apiResponse.statusCode == 200);
    assert(apiResponse.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(apiResponse.body.find("\"id\":\"C-1\"") != std::string::npos);

    handled = runtime.tryHandleGet("/api/vdr/channels", apiResponse);
    assert(!handled);
    return 0;
}
