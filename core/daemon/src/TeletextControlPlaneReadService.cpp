#include "TeletextControlPlaneReadService.h"

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

TeletextControlPlaneReadService::TeletextControlPlaneReadService(
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    ITeletextBackendAuthority& backendAuthority,
    ResolverLookup resolverLookup,
    NowProvider nowProvider)
    : backendRegistryService_(backendRegistryService),
      snapshotReadService_(snapshotReadService),
      backendAuthority_(backendAuthority),
      resolverLookup_(std::move(resolverLookup)),
      nowProvider_(nowProvider ? std::move(nowProvider) : NowProvider(systemNow))
{
}

TeletextControlPlaneReadService::FenceResult
TeletextControlPlaneReadService::fenceContext(
    const std::string& backendId,
    const std::string& channelId,
    std::uint64_t expectedBackendGeneration) const
{
    FenceResult result;

    if (backendId.empty())
    {
        result.error = "teletext_backend_id_required";
        return result;
    }
    if (channelId.empty())
    {
        result.error = "teletext_channel_id_required";
        return result;
    }

    const auto backend = backendRegistryService_.getBackend(backendId);
    if (!backend.has_value())
    {
        result.error = "teletext_backend_not_found";
        return result;
    }
    if (!backend->enabled)
    {
        result.error = "teletext_backend_disabled";
        return result;
    }

    if (!snapshotReadService_.hasSnapshotForBackend(backendId))
    {
        result.error = "teletext_backend_snapshot_unavailable";
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
        result.error = "teletext_channel_not_in_backend_snapshot";
        return result;
    }

    const TeletextBackendAuthorityState authority =
        backendAuthority_.stateForBackend(backendId, nowProvider_());
    if (!authority.present || authority.backendGeneration == 0)
    {
        result.error = "teletext_backend_generation_unavailable";
        return result;
    }
    if (!authority.online)
    {
        result.error = "teletext_backend_not_online";
        return result;
    }
    if (expectedBackendGeneration != 0 &&
        authority.backendGeneration != expectedBackendGeneration)
    {
        result.error = "teletext_backend_generation_mismatch";
        return result;
    }

    result.accepted = true;
    result.backendGeneration = authority.backendGeneration;
    return result;
}

TeletextServiceSnapshot TeletextControlPlaneReadService::discoverService(
    const std::string& backendId,
    const std::string& channelId) const
{
    const FenceResult before = fenceContext(backendId, channelId, 0);
    if (!before.accepted)
    {
        TeletextServiceSnapshot rejected;
        rejected.error = before.error;
        return rejected;
    }

    SuiteBridgeTeletextResolver* resolver =
        resolverLookup_ ? resolverLookup_(backendId) : nullptr;
    if (resolver == nullptr)
    {
        TeletextServiceSnapshot rejected;
        rejected.error = "teletext_resolver_unavailable";
        return rejected;
    }

    TeletextServiceSnapshot snapshot = resolver->discoverService(
        backendId,
        before.backendGeneration,
        channelId);

    const FenceResult after = fenceContext(
        backendId,
        channelId,
        before.backendGeneration);
    if (!after.accepted)
    {
        TeletextServiceSnapshot rejected;
        rejected.error = after.error;
        return rejected;
    }

    return snapshot;
}

TeletextPageSnapshot TeletextControlPlaneReadService::readPage(
    const TeletextServiceRef& service,
    std::uint16_t pageNumber,
    bool automaticSubpage,
    std::uint16_t subpageCode) const
{
    if (service.backendGeneration == 0)
    {
        TeletextPageSnapshot rejected;
        rejected.error = "teletext_backend_generation_unavailable";
        return rejected;
    }

    const FenceResult before = fenceContext(
        service.backendId,
        service.channelId,
        service.backendGeneration);
    if (!before.accepted)
    {
        TeletextPageSnapshot rejected;
        rejected.error = before.error;
        return rejected;
    }

    SuiteBridgeTeletextResolver* resolver =
        resolverLookup_ ? resolverLookup_(service.backendId) : nullptr;
    if (resolver == nullptr)
    {
        TeletextPageSnapshot rejected;
        rejected.error = "teletext_resolver_unavailable";
        return rejected;
    }

    TeletextPageSnapshot snapshot = resolver->readPage(
        service,
        pageNumber,
        automaticSubpage,
        subpageCode);

    const FenceResult after = fenceContext(
        service.backendId,
        service.channelId,
        service.backendGeneration);
    if (!after.accepted)
    {
        TeletextPageSnapshot rejected;
        rejected.error = after.error;
        return rejected;
    }

    return snapshot;
}
