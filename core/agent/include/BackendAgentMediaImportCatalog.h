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
    struct Preview {
        std::string sourceId;
        std::string status; // ready, duplicate, insufficient, unavailable
        std::string title;
        std::string targetFolder; // video-root-relative; never a path authority
        std::string videoCodec;
        std::uint64_t sourceBytes = 0;
        std::uint64_t availableBytes = 0;
        std::uint64_t requiredBytes = 0;
        bool videoReencodeRequired = false;
        std::string failureCode;
    };
    enum class PreviewState { denied, pending, ready };

    // One transient requested file per backend. Only a caller that already
    // passed the Core actor/grant/backend gates can initiate this.
    PreviewState requestPreview(const BackendAgentRecord& live,
                                const std::string& backendId,
                                std::int64_t now,
                                const std::string& sourceId, Preview& out)
    {
        out = {};
        if (!validSourceId(sourceId) || now <= 0 ||
            live.revoked || live.incompatible || live.leaseExpiresAt < now)
            return PreviewState::denied;
        std::lock_guard<std::mutex> guard(mutex_);
        const auto snapshot = snapshots_.find(backendId);
        if (snapshot == snapshots_.end() ||
            !liveSnapshot(snapshot->second, live, backendId, now) ||
            !std::binary_search(snapshot->second.ids.begin(),
                                snapshot->second.ids.end(), sourceId))
            return PreviewState::denied;
        auto& slot = previews_[backendId];
        if (slot.sourceId == sourceId && slot.ready &&
            slot.updatedAt <= now && now - slot.updatedAt <= 30) {
            out = slot.result;
            return PreviewState::ready;
        }
        if (slot.sourceId != sourceId || slot.requestedAt <= 0 ||
            slot.requestedAt > now || now - slot.requestedAt > 60 ||
            slot.ready)
            slot = {sourceId, now, false, 0, {}};
        return PreviewState::pending;
    }

    std::string nextPreviewRequest(const BackendAgentRecord& live,
                                   const std::string& backendId,
                                   std::int64_t now) const
    {
        std::lock_guard<std::mutex> guard(mutex_);
        const auto snapshot = snapshots_.find(backendId);
        const auto slot = previews_.find(backendId);
        if (snapshot == snapshots_.end() || slot == previews_.end() ||
            !liveSnapshot(snapshot->second, live, backendId, now) ||
            slot->second.ready || slot->second.requestedAt > now ||
            now - slot->second.requestedAt > 60 ||
            !std::binary_search(snapshot->second.ids.begin(),
                snapshot->second.ids.end(), slot->second.sourceId))
            return {};
        return slot->second.sourceId;
    }

    bool acceptPreview(const BackendAgentRecord& live,
                       const std::string& backendId,
                       std::int64_t now, const Preview& data)
    {
        if (!validSourceId(data.sourceId) ||
            (data.status != "ready" && data.status != "duplicate" &&
             data.status != "insufficient" && data.status != "unavailable") ||
            data.title.size() > 120U || data.targetFolder.size() > 512U ||
            data.videoCodec.size() > 64U ||
            data.failureCode.size() > 32U ||
            std::any_of(data.failureCode.begin(), data.failureCode.end(),
                [](unsigned char c) { return !((c >= 'a' && c <= 'z') || c == '_'); }) ||
            (data.status != "unavailable" &&
             (data.title.empty() || data.targetFolder.empty() ||
              (data.status != "duplicate" && data.videoCodec.empty()))) ||
            std::any_of(data.title.begin(), data.title.end(),
                [](unsigned char c) { return c < 0x20 || c == 0x7f; }) ||
            !validTargetFolder(data.targetFolder) ||
            std::any_of(data.videoCodec.begin(), data.videoCodec.end(),
                [](unsigned char c) {
                    return !((c >= 'a' && c <= 'z') ||
                             (c >= '0' && c <= '9') || c == '_');
                }))
            return false;
        std::lock_guard<std::mutex> guard(mutex_);
        const auto snapshot = snapshots_.find(backendId);
        const auto slot = previews_.find(backendId);
        if (snapshot == snapshots_.end() || slot == previews_.end() ||
            !liveSnapshot(snapshot->second, live, backendId, now) ||
            slot->second.ready || slot->second.sourceId != data.sourceId ||
            !std::binary_search(snapshot->second.ids.begin(),
                snapshot->second.ids.end(), data.sourceId) ||
            slot->second.requestedAt > now ||
            now - slot->second.requestedAt > 60)
            return false;
        slot->second.result = data;
        slot->second.updatedAt = now;
        slot->second.ready = true;
        return true;
    }

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
                 bool partial = true,
                 const std::vector<std::string>& folders = {})
    {
        if (!actor.authenticated() || actor.actor.type != ActorType::Agent ||
            live.actorId != actor.actor.actorId ||
            live.backendId != backendId ||
            live.agentInstanceId != instanceId ||
            live.backendGeneration != generation || generation == 0 ||
            live.revoked || live.incompatible || live.leaseExpiresAt < now ||
            now <= 0 || ids.size() > 100U || backendId.empty() ||
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
        if (!folders.empty() && (labels.empty() || folders.size() != ids.size()))
            return false;
        std::map<std::string, std::string> byFolder;
        for (std::size_t i = 0; i < folders.size(); ++i) {
            const auto& folder = folders[i];
            if (folder.size() > 240U || (!folder.empty() &&
                (folder.front() == '/' || folder.back() == '/' ||
                 folder.find('\\') != std::string::npos)))
                return false;
            std::size_t start = 0;
            while (start < folder.size()) {
                const auto stop = folder.find('/', start);
                const auto component = folder.substr(start,
                    stop == std::string::npos ? std::string::npos : stop - start);
                if (component.empty() || component == "." || component == ".." ||
                    std::any_of(component.begin(), component.end(),
                        [](unsigned char c) {
                            return c < 0x20U || c == 0x7fU;
                        })) return false;
                if (stop == std::string::npos) break;
                start = stop + 1;
            }
            if (!byFolder.emplace(ids[i], folder).second) return false;
        }
        std::lock_guard<std::mutex> guard(mutex_);
        const auto previous = snapshots_.find(backendId);
        if (previous != snapshots_.end() &&
            (previous->second.agentId != live.agentId ||
             previous->second.actorId != live.actorId ||
             previous->second.instanceId != instanceId ||
             previous->second.generation != generation))
            previews_.erase(backendId);
        snapshots_[backendId] = Snapshot{
            live.agentId, live.actorId, instanceId, generation, now,
            std::move(sorted), handlerOrigin, handlerVersion, handlerSha256,
            std::move(byId), std::move(byFolder), partial};
        return true;
    }

    bool list(const BackendAgentRecord& live,
              const std::string& backendId,
              std::int64_t now,
              std::vector<std::string>& ids,
              std::vector<std::string>& labels,
              bool* partial = nullptr,
              std::vector<std::string>* folders = nullptr) const
    {
        ids.clear();
        labels.clear();
        if (folders) folders->clear();
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
        if (folders && !current.folders.empty()) {
            for (const auto& id : ids) {
                const auto found = current.folders.find(id);
                if (found == current.folders.end()) {
                    ids.clear();
                    labels.clear();
                    folders->clear();
                    return false;
                }
                folders->push_back(found->second);
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
    static bool validTargetFolder(const std::string& path)
    {
        if (path.empty()) return true;
        if (path.front() == '/' || path.back() == '/' ||
            path.find('\\') != std::string::npos) return false;
        std::size_t at = 0;
        while (at < path.size()) {
            const auto end = path.find('/', at);
            const auto part = path.substr(at,
                end == std::string::npos ? std::string::npos : end - at);
            if (part.empty() || part == "." || part == ".." ||
                std::any_of(part.begin(), part.end(), [](unsigned char c) {
                    return c < 0x20 || c == 0x7f;
                })) return false;
            if (end == std::string::npos) break;
            at = end + 1;
        }
        return true;
    }
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
        std::map<std::string, std::string> folders;
        bool partial = true;
    };
    static bool liveSnapshot(const Snapshot& snapshot,
                             const BackendAgentRecord& live,
                             const std::string& backendId,
                             std::int64_t now)
    {
        return now > 0 && live.backendId == backendId &&
            !live.revoked && !live.incompatible &&
            live.leaseExpiresAt >= now &&
            snapshot.agentId == live.agentId &&
            snapshot.actorId == live.actorId &&
            snapshot.instanceId == live.agentInstanceId &&
            snapshot.generation == live.backendGeneration &&
            snapshot.capturedAt <= now &&
            now - snapshot.capturedAt <= 90;
    }
    struct PreviewSlot {
        std::string sourceId;
        std::int64_t requestedAt = 0;
        bool ready = false;
        std::int64_t updatedAt = 0;
        Preview result;
    };
    mutable std::mutex mutex_;
    std::map<std::string, Snapshot> snapshots_;
    std::map<std::string, PreviewSlot> previews_;
};
