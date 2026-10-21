#pragma once

#include "BackendAgentLifecycle.h"
#include "SecurityIdentity.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// Ephemeral, bounded Agent-reported observations, NOT source authority.
// Core grants and backend-local root mapping are rechecked separately.
// No filesystem locations, payloads, metadata or write operations persist.
class BackendAgentMediaImportCatalog
{
public:
    bool publish(const RequestSecurityContext& actor,
                 const BackendAgentRecord& live,
                 const std::string& backendId,
                 const std::string& instanceId,
                 std::uint64_t generation,
                 const std::vector<std::string>& ids,
                 std::int64_t now)
    {
        if (!actor.authenticated() || actor.actor.type != ActorType::Agent ||
            live.actorId != actor.actor.actorId ||
            live.backendId != backendId ||
            live.agentInstanceId != instanceId ||
            live.backendGeneration != generation || generation == 0 ||
            live.revoked || live.incompatible || live.leaseExpiresAt < now ||
            now <= 0 || ids.size() > 32U || backendId.empty() ||
            instanceId.empty()) return false;
        std::vector<std::string> sorted = ids;
        for (const auto& id : sorted)
            if (!validSourceId(id)) return false;
        std::sort(sorted.begin(), sorted.end());
        if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
            return false;
        std::lock_guard<std::mutex> guard(mutex_);
        snapshots_[backendId] = Snapshot{
            live.agentId, live.actorId, instanceId, generation, now,
            std::move(sorted)};
        return true;
    }

    bool list(const BackendAgentRecord& live,
              const std::string& backendId,
              std::int64_t now,
              std::vector<std::string>& ids) const
    {
        ids.clear();
        if (now <= 0 || live.backendId != backendId ||
            live.revoked || live.incompatible || live.leaseExpiresAt < now)
            return false;
        std::lock_guard<std::mutex> guard(mutex_);
        const auto i = snapshots_.find(backendId);
        if (i == snapshots_.end()) return false;
        const Snapshot& current = i->second;
        if (current.agentId != live.agentId ||
            current.actorId != live.actorId ||
            current.instanceId != live.agentInstanceId ||
            current.generation != live.backendGeneration ||
            current.capturedAt > now ||
            now - current.capturedAt > 90)
            return false;
        ids = current.ids;
        return true;
    }

private:
    static bool validSourceId(const std::string& id)
    {
        return id.size() >= 16U && id.size() <= 64U &&
            std::all_of(id.begin(), id.end(), [](unsigned char ch) {
                return std::isalnum(ch) || ch == '-' || ch == '_';
            });
    }
    struct Snapshot
    {
        std::string agentId;
        std::string actorId;
        std::string instanceId;
        std::uint64_t generation;
        std::int64_t capturedAt;
        std::vector<std::string> ids;
    };
    mutable std::mutex mutex_;
    std::map<std::string, Snapshot> snapshots_;
};
