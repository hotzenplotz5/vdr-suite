#include "HbbtvControlPlaneReadService.h"

#include <algorithm>
#include <ctime>
#include <utility>

namespace
{

std::int64_t systemNow()
{
    return static_cast<std::int64_t>(std::time(nullptr));
}

}

HbbtvControlPlaneReadService::HbbtvControlPlaneReadService(
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    IHbbtvBackendAuthority& backendAuthority,
    ResolverLookup resolverLookup,
    NowProvider nowProvider)
    : backendRegistryService_(backendRegistryService),
      snapshotReadService_(snapshotReadService),
      backendAuthority_(backendAuthority),
      resolverLookup_(std::move(resolverLookup)),
      nowProvider_(nowProvider ? std::move(nowProvider) : NowProvider(systemNow))
{
}

HbbtvControlPlaneReadService::FenceResult
HbbtvControlPlaneReadService::fenceContext(
    const std::string& backendId,
    const std::string& channelId,
    std::uint64_t expectedBackendGeneration) const
{
    FenceResult result;

    if (backendId.empty())
    {
        result.error = "hbbtv_backend_id_required";
        return result;
    }
    if (channelId.empty())
    {
        result.error = "hbbtv_channel_id_required";
        return result;
    }

    const auto backend = backendRegistryService_.getBackend(backendId);
    if (!backend.has_value())
    {
        result.error = "hbbtv_backend_not_found";
        return result;
    }
    if (!backend->enabled)
    {
        result.error = "hbbtv_backend_disabled";
        return result;
    }

    if (!snapshotReadService_.hasSnapshotForBackend(backendId))
    {
        result.error = "hbbtv_backend_snapshot_unavailable";
        return result;
    }

    const auto channels = snapshotReadService_.getChannelsForBackend(backendId);
    const bool channelPresent = std::any_of(
        channels.begin(),
        channels.end(),
        [&channelId](const VdrChannel& channel) {
            return channel.id == channelId;
        });
    if (!channelPresent)
    {
        result.error = "hbbtv_channel_not_in_backend_snapshot";
        return result;
    }

    const HbbtvBackendAuthorityState authority =
        backendAuthority_.stateForBackend(backendId, nowProvider_());
    if (!authority.present || authority.backendGeneration == 0)
    {
        result.error = "hbbtv_backend_generation_unavailable";
        return result;
    }
    if (!authority.online)
    {
        result.error = "hbbtv_backend_not_online";
        return result;
    }
    if (expectedBackendGeneration != 0 &&
        authority.backendGeneration != expectedBackendGeneration)
    {
        result.error = "hbbtv_backend_generation_mismatch";
        return result;
    }

    result.accepted = true;
    result.backendGeneration = authority.backendGeneration;
    return result;
}

BroadcastApplicationDiscoverySnapshot
HbbtvControlPlaneReadService::discoverApplications(
    const std::string& backendId,
    const std::string& channelId) const
{
    const FenceResult before = fenceContext(backendId, channelId, 0);
    if (!before.accepted)
    {
        BroadcastApplicationDiscoverySnapshot rejected;
        rejected.error = before.error;
        return rejected;
    }

    SuiteBridgeHbbtvResolver* resolver =
        resolverLookup_ ? resolverLookup_(backendId) : nullptr;
    if (resolver == nullptr)
    {
        BroadcastApplicationDiscoverySnapshot rejected;
        rejected.error = "hbbtv_resolver_unavailable";
        return rejected;
    }

    BroadcastApplicationDiscoverySnapshot snapshot =
        resolver->discoverApplications(
            backendId,
            before.backendGeneration,
            channelId);

    const FenceResult after = fenceContext(
        backendId,
        channelId,
        before.backendGeneration);
    if (!after.accepted)
    {
        BroadcastApplicationDiscoverySnapshot rejected;
        rejected.error = after.error;
        return rejected;
    }

    return snapshot;
}
