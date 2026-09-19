#pragma once

#include "BackendRegistryService.h"
#include "SuiteBridgeHbbtvResolver.h"
#include "VdrSnapshotReadService.h"

#include <cstdint>
#include <functional>
#include <string>

struct HbbtvBackendAuthorityState
{
    bool present = false;
    bool online = false;
    std::uint64_t backendGeneration = 0;
};

class IHbbtvBackendAuthority
{
public:
    virtual ~IHbbtvBackendAuthority() = default;

    virtual HbbtvBackendAuthorityState stateForBackend(
        const std::string& backendId,
        std::int64_t now) const = 0;
};

class IHbbtvApplicationDiscoveryService
{
public:
    virtual ~IHbbtvApplicationDiscoveryService() = default;

    virtual BroadcastApplicationDiscoverySnapshot discoverApplications(
        const std::string& backendId,
        const std::string& channelId) const = 0;
};

class HbbtvControlPlaneReadService final :
    public IHbbtvApplicationDiscoveryService
{
public:
    using ResolverLookup =
        std::function<SuiteBridgeHbbtvResolver*(const std::string&)>;
    using NowProvider = std::function<std::int64_t()>;

    HbbtvControlPlaneReadService(
        BackendRegistryService& backendRegistryService,
        VdrSnapshotReadService& snapshotReadService,
        IHbbtvBackendAuthority& backendAuthority,
        ResolverLookup resolverLookup,
        NowProvider nowProvider = {});

    BroadcastApplicationDiscoverySnapshot discoverApplications(
        const std::string& backendId,
        const std::string& channelId) const override;

private:
    struct FenceResult
    {
        bool accepted = false;
        std::uint64_t backendGeneration = 0;
        std::string error;
    };

    FenceResult fenceContext(
        const std::string& backendId,
        const std::string& channelId,
        std::uint64_t expectedBackendGeneration) const;

    BackendRegistryService& backendRegistryService_;
    VdrSnapshotReadService& snapshotReadService_;
    IHbbtvBackendAuthority& backendAuthority_;
    ResolverLookup resolverLookup_;
    NowProvider nowProvider_;
};
