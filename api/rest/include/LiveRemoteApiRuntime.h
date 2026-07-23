#pragma once

#include "DashboardController.h"
#include "BackendAccessPolicy.h"
#include "LiveOverlay.h"
#include "RemoteActionService.h"

#include <functional>
#include <string>

class IHttpClient;
class SnapshotCacheService;
class VdrSnapshotReadService;

class LiveRemoteApiRuntime
{
public:
    using ChangePublisher = std::function<void(const std::string&)>;

    static LiveRemoteApiRuntime& instance();

    void configure(
        BackendRegistryService& backendRegistryService,
        VdrSnapshotReadService& snapshotReadService,
        SnapshotCacheService& snapshotCacheService,
        ChangePublisher changePublisher,
        LiveOverlayService::EventLookup eventLookup = {});

    void registerRestfulApiBackend(
        const std::string& backendId,
        IHttpClient& httpClient);

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        ApiResponse& response) const;

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

    bool configured() const;
    void reset();

private:
    LiveRemoteApiRuntime() = default;

    static std::string pathPart(const std::string& requestTarget);
    static std::string queryValue(
        const std::string& requestTarget,
        const std::string& name);
    static std::string percentDecode(const std::string& value);

    BackendRegistryService* backendRegistryService_ = nullptr;
    VdrSnapshotReadService* snapshotReadService_ = nullptr;
    SnapshotCacheService* snapshotCacheService_ = nullptr;
    ChangePublisher changePublisher_;
    LiveOverlayService::EventLookup eventLookup_;
    BackendAccessPolicy backendAccessPolicy_;
    RemoteActionExecutorRegistry remoteActionExecutorRegistry_;
    LiveChannelStateProviderRegistry liveChannelStateProviderRegistry_;
};
