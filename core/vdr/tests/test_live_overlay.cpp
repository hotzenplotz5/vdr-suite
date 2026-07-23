#include "BackendRegistry.h"
#include "LiveOverlay.h"
#include "SnapshotAccessService.h"
#include "SnapshotCache.h"

#include <cassert>
#include <memory>

class StaticLiveChannelProvider : public ILiveChannelStateProvider
{
public:
    explicit StaticLiveChannelProvider(const LiveChannelState& state)
        : state_(state)
    {
    }

    LiveChannelState getState() const override
    {
        return state_;
    }

private:
    LiveChannelState state_;
};

int main()
{
    BackendNode node;
    node.backendId = "default";
    node.backendName = "Default";
    node.accessMode = "read-only";
    node.enabled = true;
    node.capabilities.liveOverlayRead = true;

    BackendRegistry registry;
    registry.addBackend(node);
    BackendRegistryService registryService(registry);

    VdrSnapshot snapshot;
    snapshot.backendId = "default";
    VdrChannel channel;
    channel.id = "C-1";
    channel.number = 2;
    channel.name = "ZDF HD";
    snapshot.channels.push_back(channel);

    VdrEvent present;
    present.id = "81724";
    present.channelId = "C-1";
    present.title = "heute journal";
    present.startTime = "100";
    present.endTime = "200";
    snapshot.events.push_back(present);

    VdrEvent following;
    following.id = "81725";
    following.channelId = "C-1";
    following.title = "Wetter";
    following.startTime = "200";
    following.endTime = "240";
    snapshot.events.push_back(following);

    VdrTimer timer;
    timer.channelId = "C-1";
    timer.eventId = "81724";
    timer.enabled = true;
    snapshot.timers.push_back(timer);

    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    cacheService.updateSnapshotForBackend("default", snapshot);
    SnapshotAccessService accessService(cacheService);
    VdrSnapshotReadService readService(accessService);

    LiveChannelStateProviderRegistry providers;
    providers.registerProvider(
        "default",
        std::make_shared<StaticLiveChannelProvider>(
            LiveChannelState{true, "C-1", "available"}));

    LiveOverlayService service(registryService, readService, cacheService, providers);
    LiveOverlaySnapshot result = service.getSnapshot("default", 150);
    assert(result.success);
    assert(result.channel.available);
    assert(result.channel.number == 2);
    assert(result.present.eventId == "81724");
    assert(result.following.eventId == "81725");
    assert(result.timer.active);
    assert(!result.audio.available);

    result = service.getSnapshot("missing", 150);
    assert(!result.success);
    assert(result.statusCode == 404);

    VdrSnapshot noEpgSnapshot;
    noEpgSnapshot.backendId = "secondary";
    VdrChannel secondaryChannel;
    secondaryChannel.id = "C-2";
    secondaryChannel.number = 7;
    secondaryChannel.name = "Secondary";
    noEpgSnapshot.channels.push_back(secondaryChannel);
    cacheService.updateSnapshotForBackend("secondary", noEpgSnapshot);

    BackendNode secondary = node;
    secondary.backendId = "secondary";
    secondary.backendName = "Secondary";
    registry.addBackend(secondary);
    providers.registerProvider(
        "secondary",
        std::make_shared<StaticLiveChannelProvider>(
            LiveChannelState{true, "C-2", "available"}));

    result = service.getSnapshot("secondary", 150);
    assert(result.success);
    assert(result.channel.id == "C-2");
    assert(!result.present.available);
    assert(!result.following.available);
    assert(result.backendId == "secondary");
    return 0;
}
