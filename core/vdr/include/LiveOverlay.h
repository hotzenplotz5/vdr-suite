#pragma once

#include "BackendRegistryService.h"
#include "SnapshotCacheService.h"
#include "VdrEvent.h"
#include "VdrSnapshotReadService.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct LiveChannelState
{
    bool available = false;
    std::string channelId;
    std::string message;
};

class ILiveChannelStateProvider
{
public:
    virtual ~ILiveChannelStateProvider() = default;
    virtual LiveChannelState getState() const = 0;
};

class LiveChannelStateProviderRegistry
{
public:
    void registerProvider(
        const std::string& backendId,
        std::shared_ptr<ILiveChannelStateProvider> provider);

    std::shared_ptr<ILiveChannelStateProvider> findProvider(
        const std::string& backendId) const;

    void clear();

private:
    std::map<std::string, std::shared_ptr<ILiveChannelStateProvider>> providers_;
};

struct LiveOverlayChannel
{
    bool available = false;
    std::string id;
    int number = 0;
    std::string name;
};

struct LiveOverlayEvent
{
    bool available = false;
    std::string eventId;
    std::string title;
    std::string subtitle;
    long long startTime = 0;
    long long endTime = 0;
};

struct LiveOverlayTimerState
{
    bool active = false;
    bool recording = false;
};

struct LiveOverlayAudioState
{
    bool available = false;
};

struct LiveOverlaySnapshot
{
    bool success = false;
    int statusCode = 200;
    std::string backendId;
    int revision = 0;
    long long generatedAt = 0;
    std::string message;
    LiveOverlayChannel channel;
    LiveOverlayEvent present;
    LiveOverlayEvent following;
    LiveOverlayTimerState timer;
    LiveOverlayAudioState audio;
};

class LiveOverlayService
{
public:
    using EventLookup = std::function<std::vector<VdrEvent>(
        const std::string& backendId,
        const std::string& channelId,
        long long fromEpoch,
        int eventLimit)>;

    LiveOverlayService(
        BackendRegistryService& backendRegistryService,
        VdrSnapshotReadService& snapshotReadService,
        SnapshotCacheService& snapshotCacheService,
        const LiveChannelStateProviderRegistry& providerRegistry,
        EventLookup eventLookup = {});

    LiveOverlaySnapshot getSnapshot(
        const std::string& backendId,
        long long now = 0) const;

private:
    BackendRegistryService& backendRegistryService_;
    VdrSnapshotReadService& snapshotReadService_;
    SnapshotCacheService& snapshotCacheService_;
    const LiveChannelStateProviderRegistry& providerRegistry_;
    EventLookup eventLookup_;
};
