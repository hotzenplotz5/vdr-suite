#pragma once

#include "HbbtvControlPlaneReadService.h"
#include "HbbtvDomain.h"
#include "SuiteBridgeHbbtvRuntimeResolver.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

struct BroadcastApplicationLaunchRequest
{
    std::string actorId;
    std::string clientContext;
    std::string correlationContext;
    BroadcastApplicationRef application;
    std::int64_t lifetimeSeconds = 3600;
};

struct BroadcastApplicationSessionResult
{
    bool accepted = false;
    std::string error;
    BroadcastApplicationSession session;
};

class HbbtvApplicationSessionService
{
public:
    using RuntimeLookup =
        std::function<IHbbtvRuntimeControl*(const std::string& backendId)>;
    using AuthorizationCheck = std::function<bool(
        const std::string& permission,
        const std::string& actorId,
        const std::string& backendId)>;
    using SessionIdFactory = std::function<std::string()>;
    using NowProvider = std::function<std::int64_t()>;

    HbbtvApplicationSessionService(
        IHbbtvApplicationDiscoveryService& discoveryService,
        RuntimeLookup runtimeLookup,
        AuthorizationCheck authorizationCheck,
        SessionIdFactory sessionIdFactory = {},
        NowProvider nowProvider = {});

    BroadcastApplicationSessionResult launch(
        const BroadcastApplicationLaunchRequest& request);

    BroadcastApplicationSessionResult refresh(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& clientContext);

    BroadcastApplicationSessionResult authorizePresentation(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& clientContext);

    BroadcastApplicationSessionResult input(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& clientContext,
        SuiteBridgeHbbtvInputAction action);

    BroadcastApplicationSessionResult close(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& clientContext,
        const std::string& reason);

    std::optional<BroadcastApplicationSession> find(
        const std::string& sessionId) const;

private:
    BroadcastApplicationSessionResult reject(
        const std::string& error) const;

    bool authorized(
        const char* permission,
        const BroadcastApplicationSession& session) const;

    bool owns(
        const BroadcastApplicationSession& session,
        const std::string& actorId,
        const std::string& clientContext) const;

    bool applicationCurrent(
        const BroadcastApplicationRef& application) const;

    IHbbtvRuntimeControl* runtimeFor(
        const BroadcastApplicationSession& session) const;

    void store(const BroadcastApplicationSession& session);

    IHbbtvApplicationDiscoveryService& discoveryService_;
    RuntimeLookup runtimeLookup_;
    AuthorizationCheck authorizationCheck_;
    SessionIdFactory sessionIdFactory_;
    NowProvider nowProvider_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, BroadcastApplicationSession> sessions_;
};
