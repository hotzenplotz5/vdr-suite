#pragma once

#include "BackendAccessPolicy.h"
#include "LegacyOsdControllerDomain.h"
#include "SecurityIdentity.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class LegacyOsdSessionService;
class OsdViewerBindingService;

struct OsdControllerAcquireRequest
{
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::string legacyOsdSessionId;
    std::string viewerBindingId;
};

struct OsdControllerRenewRequest
{
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::string legacyOsdSessionId;
    std::string viewerBindingId;
    std::string controllerLeaseId;
    std::uint64_t controllerLeaseEpoch = 0;
    std::uint64_t leaseRevision = 0;
};

struct OsdControllerReleaseRequest
{
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::string legacyOsdSessionId;
    std::string viewerBindingId;
    std::string controllerLeaseId;
    std::uint64_t controllerLeaseEpoch = 0;
};

struct OsdControllerStatusRequest
{
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::string legacyOsdSessionId;
    std::string viewerBindingId;
};

struct OsdControllerLeaseResult
{
    bool accepted = false;
    bool hasLease = false;
    std::string error;
    OsdControllerLease lease;
};

class OsdControllerLeaseService
{
public:
    static constexpr std::int64_t LeaseLifetimeSeconds = 30;
    static constexpr std::int64_t RenewAfterSeconds = 10;
    static constexpr std::size_t MaximumLeases = 64;
    static constexpr std::size_t MaximumSurfaceScopes = 64;

    using ContextResolver = std::function<std::optional<RequestSecurityContext>(
        const std::string&, const std::string&)>;
    using BackendPolicyLookup = std::function<BackendAccessDecision(
        const std::string&)>;
    using LeaseIdFactory = std::function<std::string()>;
    using NowProvider = std::function<std::int64_t()>;

    OsdControllerLeaseService(
        LegacyOsdSessionService& sessionService,
        OsdViewerBindingService& viewerService,
        ContextResolver contextResolver,
        BackendPolicyLookup backendPolicyLookup,
        LeaseIdFactory leaseIdFactory = {},
        NowProvider nowProvider = {});

    OsdControllerLeaseResult acquire(
        const OsdControllerAcquireRequest& request);
    OsdControllerLeaseResult renew(
        const OsdControllerRenewRequest& request);
    OsdControllerLeaseResult release(
        const OsdControllerReleaseRequest& request);
    OsdControllerLeaseResult current(
        const OsdControllerStatusRequest& request);
    OsdControllerLeaseResult revoke(
        const std::string& controllerLeaseId,
        const std::string& reason);
    std::optional<OsdControllerLease> find(
        const std::string& controllerLeaseId) const;

private:
    struct HolderScope
    {
        std::uint64_t backendGeneration = 0;
        std::string osdSurfaceId;
        std::string osdEpoch;
        std::int64_t expiresAt = 0;
    };

    OsdControllerLeaseResult reject(const std::string& error) const;
    bool authorizeControl(
        const std::string& actorId,
        const std::string& backendId,
        std::string& error) const;
    bool resolveViewScope(
        const std::string& actorId,
        const std::string& clientInstanceId,
        const std::string& backendId,
        const std::string& legacyOsdSessionId,
        const std::string& viewerBindingId,
        std::int64_t now,
        HolderScope& scope,
        std::string& error) const;
    std::optional<OsdControllerLease> refreshLease(
        const std::string& controllerLeaseId,
        std::int64_t now);
    OsdControllerLease endLease(
        const OsdControllerLease& snapshot,
        OsdControllerLeaseState state,
        const std::string& reason);
    static std::string surfaceKey(
        const std::string& backendId,
        std::uint64_t backendGeneration,
        const std::string& osdSurfaceId,
        const std::string& osdEpoch);
    void removeOwnerLocked(const OsdControllerLease& lease);
    void reapExpiredLocked(std::int64_t now);
    void pruneTerminalLocked();
    void pruneSurfaceEpochsLocked();

    LegacyOsdSessionService& sessionService_;
    OsdViewerBindingService& viewerService_;
    ContextResolver contextResolver_;
    BackendPolicyLookup backendPolicyLookup_;
    LeaseIdFactory leaseIdFactory_;
    NowProvider nowProvider_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, OsdControllerLease> leases_;
    std::unordered_map<std::string, std::string> surfaceOwners_;
    std::unordered_map<std::string, std::uint64_t> surfaceEpochs_;
};
