#pragma once

#include "BackendRegistryService.h"
#include "SuiteBridgeTeletextResolver.h"
#include "VdrSnapshotReadService.h"

#include <cstdint>
#include <functional>
#include <string>

struct TeletextBackendAuthorityState
{
    bool present = false;
    bool online = false;
    std::uint64_t backendGeneration = 0;
};

class ITeletextBackendAuthority
{
public:
    virtual ~ITeletextBackendAuthority() = default;

    virtual TeletextBackendAuthorityState stateForBackend(
        const std::string& backendId,
        std::int64_t now) const = 0;
};

class TeletextControlPlaneReadService
{
public:
    using ResolverLookup =
        std::function<SuiteBridgeTeletextResolver*(const std::string&)>;
    using NowProvider = std::function<std::int64_t()>;

    TeletextControlPlaneReadService(
        BackendRegistryService& backendRegistryService,
        VdrSnapshotReadService& snapshotReadService,
        ITeletextBackendAuthority& backendAuthority,
        ResolverLookup resolverLookup,
        NowProvider nowProvider = {});

    TeletextServiceSnapshot discoverService(
        const std::string& backendId,
        const std::string& channelId) const;

    TeletextPageSnapshot readPage(
        const TeletextServiceRef& service,
        std::uint16_t pageNumber,
        bool automaticSubpage = true,
        std::uint16_t subpageCode = 0) const;

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
    ITeletextBackendAuthority& backendAuthority_;
    ResolverLookup resolverLookup_;
    NowProvider nowProvider_;
};
