#include "LiveOverlay.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <utility>

namespace
{
long long parseEpoch(const std::string& value)
{
    if (value.empty())
    {
        return 0;
    }

    char* end = nullptr;
    const long long epoch = std::strtoll(value.c_str(), &end, 10);
    return end != value.c_str() && end != nullptr && *end == '\0'
        ? epoch
        : 0;
}

LiveOverlayEvent toOverlayEvent(const VdrEvent& event)
{
    LiveOverlayEvent overlay;
    overlay.available = true;
    overlay.eventId = event.id;
    overlay.title = event.title;
    overlay.subtitle = event.subtitle;
    overlay.startTime = parseEpoch(event.startTime);
    overlay.endTime = parseEpoch(event.endTime);

    if (overlay.endTime <= overlay.startTime && event.durationSeconds > 0)
    {
        overlay.endTime = overlay.startTime + event.durationSeconds;
    }

    return overlay;
}
}

void LiveChannelStateProviderRegistry::registerProvider(
    const std::string& backendId,
    std::shared_ptr<ILiveChannelStateProvider> provider)
{
    if (backendId.empty() || !provider)
    {
        return;
    }

    providers_[backendId] = std::move(provider);
}

std::shared_ptr<ILiveChannelStateProvider> LiveChannelStateProviderRegistry::findProvider(
    const std::string& backendId) const
{
    const auto iterator = providers_.find(backendId);
    return iterator == providers_.end() ? nullptr : iterator->second;
}

void LiveChannelStateProviderRegistry::clear()
{
    providers_.clear();
}

LiveOverlayService::LiveOverlayService(
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    SnapshotCacheService& snapshotCacheService,
    const LiveChannelStateProviderRegistry& providerRegistry)
    : backendRegistryService_(backendRegistryService),
      snapshotReadService_(snapshotReadService),
      snapshotCacheService_(snapshotCacheService),
      providerRegistry_(providerRegistry)
{
}

LiveOverlaySnapshot LiveOverlayService::getSnapshot(
    const std::string& backendId,
    long long requestedNow) const
{
    LiveOverlaySnapshot snapshot;
    snapshot.backendId = backendId;
    snapshot.revision = snapshotCacheService_.generation();
    snapshot.generatedAt = requestedNow > 0
        ? requestedNow
        : static_cast<long long>(std::time(nullptr));

    if (backendId.empty())
    {
        snapshot.statusCode = 400;
        snapshot.message = "backendId is required";
        return snapshot;
    }

    const auto backend = backendRegistryService_.getBackend(backendId);

    if (!backend.has_value())
    {
        snapshot.statusCode = 404;
        snapshot.message = "backend not found";
        return snapshot;
    }

    if (!backend->enabled)
    {
        snapshot.statusCode = 503;
        snapshot.message = "backend is disabled";
        return snapshot;
    }

    if (!backend->capabilities.liveOverlayRead)
    {
        snapshot.statusCode = 409;
        snapshot.message = "live.overlay.read capability is unavailable";
        return snapshot;
    }

    if (!snapshotReadService_.hasSnapshotForBackend(backendId))
    {
        snapshot.statusCode = 503;
        snapshot.message = "backend snapshot is unavailable";
        return snapshot;
    }

    snapshot.success = true;

    const auto provider = providerRegistry_.findProvider(backendId);

    if (!provider)
    {
        snapshot.message = "live channel source is unavailable";
        return snapshot;
    }

    const LiveChannelState state = provider->getState();

    if (!state.available || state.channelId.empty())
    {
        snapshot.message = state.message.empty()
            ? "live channel is unavailable"
            : state.message;
        return snapshot;
    }

    const std::vector<VdrChannel> channels =
        snapshotReadService_.getChannelsForBackend(backendId);

    const auto channel = std::find_if(
        channels.begin(),
        channels.end(),
        [&state](const VdrChannel& candidate)
        {
            return candidate.id == state.channelId;
        });

    if (channel == channels.end())
    {
        snapshot.message = "live channel is not present in the Suite snapshot";
        return snapshot;
    }

    snapshot.channel.available = true;
    snapshot.channel.id = channel->id;
    snapshot.channel.number = channel->number;
    snapshot.channel.name = channel->name;

    const std::vector<VdrEvent> events =
        snapshotReadService_.getEventsForBackend(backendId);

    const VdrEvent* present = nullptr;
    const VdrEvent* following = nullptr;
    long long followingStart = std::numeric_limits<long long>::max();

    for (const VdrEvent& event : events)
    {
        if (event.channelId != state.channelId)
        {
            continue;
        }

        const long long start = parseEpoch(event.startTime);
        long long end = parseEpoch(event.endTime);

        if (end <= start && event.durationSeconds > 0)
        {
            end = start + event.durationSeconds;
        }

        if (start > 0 && start <= snapshot.generatedAt && end > snapshot.generatedAt)
        {
            present = &event;
            continue;
        }

        if (start > snapshot.generatedAt && start < followingStart)
        {
            followingStart = start;
            following = &event;
        }
    }

    if (present != nullptr)
    {
        snapshot.present = toOverlayEvent(*present);
    }

    if (following != nullptr)
    {
        snapshot.following = toOverlayEvent(*following);
    }

    const std::vector<VdrTimer> timers =
        snapshotReadService_.getTimersForBackend(backendId);

    for (const VdrTimer& timer : timers)
    {
        const bool eventMatch =
            snapshot.present.available &&
            !snapshot.present.eventId.empty() &&
            timer.eventId == snapshot.present.eventId;
        const bool recordingOnChannel =
            timer.recording &&
            timer.channelId == state.channelId;

        if (!eventMatch && !recordingOnChannel)
        {
            continue;
        }

        snapshot.timer.active =
            snapshot.timer.active ||
            (eventMatch && timer.enabled) ||
            recordingOnChannel;
        snapshot.timer.recording =
            snapshot.timer.recording || recordingOnChannel;
    }

    snapshot.message = snapshot.present.available
        ? "Live overlay snapshot available"
        : "Live channel available; present EPG event unavailable";

    return snapshot;
}
