#pragma once

#include "BackendAgentLifecycle.h"
#include "LegacyOsdDomain.h"
#include "SecurityIdentity.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

struct LegacyOsdSessionCreateRequest
{
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::string correlationId;
};

struct LegacyOsdSessionResult
{
    bool accepted = false;
    std::string error;
    LegacyOsdSession session;
};

class LegacyOsdSessionService
{
public:
    static constexpr std::int64_t SessionLifetimeSeconds = 300;
    static constexpr std::size_t MaximumSessions = 64;

    using ContextResolver = std::function<std::optional<RequestSecurityContext>(
        const std::string&, const std::string&)>;
    using StatusLookup = std::function<BackendAgentStatus(
        const std::string&, std::int64_t)>;
    using ObservationRead = std::function<BackendAgentOsdReadResult(
        const RequestSecurityContext&, const std::string&,
        std::uint64_t, std::int64_t)>;
    using SessionIdFactory = std::function<std::string()>;
    using NowProvider = std::function<std::int64_t()>;

    LegacyOsdSessionService(
        ContextResolver contextResolver,
        StatusLookup statusLookup,
        ObservationRead observationRead,
        SessionIdFactory sessionIdFactory = {},
        NowProvider nowProvider = {});

    LegacyOsdSessionResult create(
        const LegacyOsdSessionCreateRequest& request);
    LegacyOsdSessionResult status(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& clientInstanceId,
        const std::string& backendId);
    std::optional<LegacyOsdSession> find(
        const std::string& sessionId) const;

private:
    LegacyOsdSessionResult reject(const std::string& error) const;
    std::optional<RequestSecurityContext> authorizedContext(
        const std::string& actorId,
        const std::string& backendId) const;
    static bool currentOrResync(const BackendAgentOsdReadResult& read);
    static void applyObservation(
        LegacyOsdSession& session,
        const BackendAgentOsdReadResult& read);
    void store(const LegacyOsdSession& session);
    void reapExpired(std::int64_t now);

    ContextResolver contextResolver_;
    StatusLookup statusLookup_;
    ObservationRead observationRead_;
    SessionIdFactory sessionIdFactory_;
    NowProvider nowProvider_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, LegacyOsdSession> sessions_;
};
