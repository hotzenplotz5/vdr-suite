#pragma once

#include "BackendAgentLifecycle.h"
#include "AddonRootApproval.h"
#include "SecurityIdentity.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <utility>

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
                 std::int64_t now,
                 const std::string& handlerOrigin = {},
                 const std::string& handlerVersion = {},
                 const std::string& handlerSha256 = {},
                 const std::vector<std::string>& labels = {},
                 bool partial = true)
    {
        if (!actor.authenticated() || actor.actor.type != ActorType::Agent ||
            live.actorId != actor.actor.actorId ||
            live.backendId != backendId ||
            live.agentInstanceId != instanceId ||
            live.backendGeneration != generation || generation == 0 ||
            live.revoked || live.incompatible || live.leaseExpiresAt < now ||
            now <= 0 || ids.size() > 256U || backendId.empty() ||
            instanceId.empty()) return false;
        const bool absent = handlerOrigin.empty() &&
            handlerVersion.empty() && handlerSha256.empty();
        const bool validDigest = handlerSha256.size() == 64U &&
            std::all_of(handlerSha256.begin(), handlerSha256.end(),
                [](unsigned char c) {
                    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
                });
        if (!absent &&
            ((handlerOrigin != "manual" && handlerOrigin != "debian") ||
             handlerVersion != "0.1.0" || !validDigest)) return false;
        std::vector<std::string> sorted = ids;
        for (const auto& id : sorted)
            if (!validSourceId(id)) return false;
        std::sort(sorted.begin(), sorted.end());
        if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
            return false;
        if (!labels.empty() && labels.size() != ids.size()) return false;
        std::map<std::string, std::string> byId;
        for (std::size_t i = 0; i < labels.size(); ++i) {
            const auto& label = labels[i];
            if (label.empty() || label.size() > 160U ||
                std::any_of(label.begin(), label.end(), [](unsigned char ch) {
                    return ch < 0x20 || ch == 0x7f ||
                           ch == '/' || ch == '\\';
                })) return false;
            if (!byId.emplace(ids[i], label).second) return false;
        }
        std::lock_guard<std::mutex> guard(mutex_);
        snapshots_[backendId] = Snapshot{
            live.agentId, live.actorId, instanceId, generation, now,
            std::move(sorted), handlerOrigin, handlerVersion, handlerSha256,
            std::move(byId), partial};
        return true;
    }

    bool list(const BackendAgentRecord& live,
              const std::string& backendId,
              std::int64_t now,
              std::vector<std::string>& ids,
              std::vector<std::string>& labels,
              bool* partial = nullptr) const
    {
        ids.clear();
        labels.clear();
        if (partial) *partial = true;
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
        if (partial) *partial = current.partial;
        if (!current.labels.empty()) {
            for (const auto& id : ids) {
                const auto found = current.labels.find(id);
                if (found == current.labels.end()) {
                    ids.clear();
                    labels.clear();
                    return false;
                }
                labels.push_back(found->second);
            }
        }
        return true;
    }

    bool list(const BackendAgentRecord& live,
              const std::string& backendId,
              std::int64_t now,
              std::vector<std::string>& ids) const
    {
        std::vector<std::string> ignored;
        return list(live, backendId, now, ids, ignored);
    }

    // An authenticated Agent observation bound to its live lease. This is
    // not independent Core authorization, and never activates a handler.
    bool observedInstallation(const BackendAgentRecord& live,
                              const std::string& backendId,
                              std::int64_t now,
                              std::string& origin,
                              std::string& version,
                              std::string& sha256) const
    {
        origin.clear(); version.clear(); sha256.clear();
        if (now <= 0 || live.backendId != backendId || live.revoked ||
            live.incompatible || live.leaseExpiresAt < now) return false;
        std::lock_guard<std::mutex> guard(mutex_);
        const auto found = snapshots_.find(backendId);
        if (found == snapshots_.end()) return false;
        const auto& current = found->second;
        if (current.agentId != live.agentId ||
            current.actorId != live.actorId ||
            current.instanceId != live.agentInstanceId ||
            current.generation != live.backendGeneration ||
            current.capturedAt > now || now - current.capturedAt > 90 ||
            current.handlerSha256.empty()) return false;
        origin = current.handlerOrigin;
        version = current.handlerVersion;
        sha256 = current.handlerSha256;
        return true;
    }


    // Core must supply its OWN independently verified root-admin approval.
    // Neither Agent metadata nor a matching self-reported digest is authority.
    // This read-only check does not enable an add-on or authorize dispatch.
    bool matchesIndependentCoreApproval(
        const BackendAgentRecord& live,
        const std::string& backendId,
        std::int64_t now,
        const vdrsuite::addons::RootApprovalResult& coreApproval) const
    {
        using vdrsuite::addons::InstallOrigin;
        if (!coreApproval.approved() ||
            (coreApproval.origin != InstallOrigin::manual &&
             coreApproval.origin != InstallOrigin::debian)) return false;
        std::string origin, version, sha256;
        if (!observedInstallation(live, backendId, now, origin, version, sha256))
            return false;
        return origin == (coreApproval.origin == InstallOrigin::manual
                              ? "manual" : "debian") &&
            version == coreApproval.version &&
            sha256 == coreApproval.sha256;
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
        std::string handlerOrigin;
        std::string handlerVersion;
        std::string handlerSha256;
        std::map<std::string, std::string> labels;
        bool partial = true;
    };
    mutable std::mutex mutex_;
    std::map<std::string, Snapshot> snapshots_;
};
