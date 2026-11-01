#pragma once

#include "BackendAgentLifecycle.h"
#include "AddonRootApproval.h"
#include "SecurityIdentity.h"
#include "Database.h"
#include <sqlite3.h>
#include <array>
#include <limits>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <utility>

// Bounded Agent-reported observations, NOT source authority.
// Only successful observations for content-identity scan_ sources persist in
// the existing Suite database. Core still checks grants and actual mutations.
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
        std::string nfoOutline, nfoPlot, nfoYear, nfoGenre;
        std::uint64_t durationSeconds = 0;
        std::string posterDataUri, fanartDataUri;
    };
    enum class PreviewState { denied, pending, ready };

    BackendAgentMediaImportCatalog() = default;
    explicit BackendAgentMediaImportCatalog(Database& database)
        : database_(&database)
    {
        // A read-only UI cache must never prevent daemon startup.
        persistent_ = database_->execute(
            "CREATE TABLE IF NOT EXISTS media_import_checked_previews ("
            "backend_id TEXT NOT NULL, source_id TEXT NOT NULL, "
            "payload BLOB NOT NULL, PRIMARY KEY(backend_id,source_id));");
    }

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
        // Reuse observations from this Agent generation, also after the
        // browser has been closed. The source token binds the file identity;
        // Core still revalidates every subsequent import independently.
        const auto history = checked_.find(backendId);
        if (history != checked_.end()) {
            const auto known = history->second.find(sourceId);
            if (known != history->second.end() &&
                known->second.first <= now &&
                // Completed codec checks are tied to the source fingerprint
                // embedded in sourceId. Never redo FFprobe merely because an
                // arbitrary hour has passed; a changed source receives a new
                // ID on the next inventory scan. Imports still independently
                // validate the source and destination.
                (known->second.second.status == "ready" ||
                 now - known->second.first <=
                     (known->second.second.status == "unavailable" ||
                      known->second.second.status == "insufficient" ? 60 : 3600))) {
                out = known->second.second;
                return PreviewState::ready;
            }
        }
        Preview remembered;
        if (persistentPreview(backendId, sourceId, remembered)) {
            checked_[backendId][sourceId] = {now, remembered};
            out = std::move(remembered);
            return PreviewState::ready;
        }
        auto& slot = previews_[backendId];
        if (slot.sourceId != sourceId || slot.requestedAt <= 0 ||
            slot.requestedAt > now || now - slot.requestedAt > 60 ||
            slot.ready)
            slot = {sourceId, now, false, 0, {}};
        // A selected film overrides the idle background slot immediately.
        attempted_[backendId].insert(sourceId);
        return PreviewState::pending;
    }

    std::string nextPreviewRequest(const BackendAgentRecord& live,
                                   const std::string& backendId,
                                   std::int64_t now)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        const auto snapshot = snapshots_.find(backendId);
        if (snapshot == snapshots_.end() ||
            !liveSnapshot(snapshot->second, live, backendId, now))
            return {};
        auto& slot = previews_[backendId];
        if (!slot.ready && slot.requestedAt > 0 &&
            slot.requestedAt <= now && now - slot.requestedAt <= 60 &&
            std::binary_search(snapshot->second.ids.begin(),
                               snapshot->second.ids.end(), slot.sourceId))
            return slot.sourceId;

        // When no manual preview is pending, use the existing heartbeat
        // observation for the next unexamined source. Never dispatch jobs.
        auto& attempted = attempted_[backendId];
        const auto& checked = checked_[backendId];
        for (const auto& id : snapshot->second.ids) {
            if (attempted.count(id) || checked.count(id)) continue;
            const auto state = snapshot->second.statuses.find(id);
            if (state != snapshot->second.statuses.end() &&
                state->second == "recording_present") continue;
            const auto label = snapshot->second.labels.find(id);
            if (label != snapshot->second.labels.end()) {
                const auto& name = label->second;
                if ((name.size() >= 10 &&
                     name.compare(name.size() - 10, 10, ".pc_encode") == 0) ||
                    (name.size() >= 8 &&
                     name.compare(name.size() - 8, 8, ".skipped") == 0) ||
                    (name.size() >= 10 &&
                     name.compare(name.size() - 10, 10, ".duplicate") == 0))
                    continue;
            }
            slot = {id, now, false, 0, {}};
            attempted.insert(id);
            return id;
        }
        return {};
    }

    bool acceptPreview(const BackendAgentRecord& live,
                       const std::string& backendId,
                       std::int64_t now, const Preview& data)
    {
        const auto validMetadata = [](const std::string& value) {
            return value.size() <= 4096U &&
                std::none_of(value.begin(), value.end(),
                    [](unsigned char c) { return c < 0x20 || c == 0x7f; });
        };
        const auto validArtwork = [](const std::string& uri) {
            if (uri.empty()) return true;
            if (uri.rfind("data:image/jpeg;base64,/9j/", 0) != 0 &&
                uri.rfind("data:image/png;base64,iVBORw0KGgo", 0) != 0 &&
                uri.rfind("data:image/webp;base64,UklGR", 0) != 0)
                return false;
            const auto comma = uri.find(',');
            if (comma == std::string::npos || uri.size() - comma - 1 > 349528U ||
                (uri.size() - comma - 1) % 4 != 0) return false;
            const auto encoded = uri.substr(comma + 1);
            const auto pad = encoded.find('=');
            if (pad != std::string::npos && encoded.size() - pad > 2) return false;
            return std::all_of(encoded.begin(), encoded.end(),
                [](unsigned char ch) {
                    return (ch >= 'A' && ch <= 'Z') ||
                        (ch >= 'a' && ch <= 'z') ||
                        (ch >= '0' && ch <= '9') ||
                        ch == '+' || ch == '/' || ch == '=';
                });
        };
        if (!validArtwork(data.posterDataUri) ||
            !validArtwork(data.fanartDataUri)) return false;
        if (!validMetadata(data.nfoOutline) ||
            !validMetadata(data.nfoPlot) ||
            !validMetadata(data.nfoGenre) ||
            (!data.nfoYear.empty() &&
             (data.nfoYear.size() != 4U ||
              !std::all_of(data.nfoYear.begin(), data.nfoYear.end(),
                  [](unsigned char c) { return c >= '0' && c <= '9'; }))) ||
            data.durationSeconds > 4294967295ULL)
            return false;
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
        checked_[backendId][data.sourceId] = {now, data};
        if (data.status == "ready")
            rememberPreview(backendId, data);
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
                 const std::vector<std::string>& folders = {},
                 const std::vector<std::string>& statuses = {})
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
        if (!statuses.empty() &&
            (labels.empty() || statuses.size() != ids.size())) return false;
        std::map<std::string, std::string> byStatus;
        for (std::size_t i = 0; i < statuses.size(); ++i) {
            if (statuses[i] != "recording_present" && statuses[i] != "unverified")
                return false;
            if (!byStatus.emplace(ids[i], statuses[i]).second) return false;
        }
        std::lock_guard<std::mutex> guard(mutex_);
        const auto previous = snapshots_.find(backendId);
        if (previous != snapshots_.end() &&
            (previous->second.agentId != live.agentId ||
             previous->second.actorId != live.actorId ||
             previous->second.instanceId != instanceId ||
             previous->second.generation != generation)) {
            previews_.erase(backendId);
            checked_.erase(backendId);
            attempted_.erase(backendId);
        }
        // A replaced/deleted source cannot retain its old observation.
        // The catalog remains bounded by the 100 authenticated source IDs.
        auto& checked = checked_[backendId];
        auto& attempted = attempted_[backendId];
        for (auto it = checked.begin(); it != checked.end();) {
            if (!std::binary_search(sorted.begin(), sorted.end(), it->first))
                it = checked.erase(it);
            else ++it;
        }
        for (auto it = attempted.begin(); it != attempted.end();) {
            if (!std::binary_search(sorted.begin(), sorted.end(), *it))
                it = attempted.erase(it);
            else ++it;
        }
        // Restore already completed technical checks immediately on inventory
        // publication, without another Agent FFprobe or browser request.
        for (const auto& id : sorted) {
            if (checked.count(id)) continue;
            Preview remembered;
            if (persistentPreview(backendId, id, remembered))
                checked[id] = {now, std::move(remembered)};
        }
        snapshots_[backendId] = Snapshot{
            live.agentId, live.actorId, instanceId, generation, now,
            std::move(sorted), handlerOrigin, handlerVersion, handlerSha256,
            std::move(byId), std::move(byFolder), partial,
            std::move(byStatus)};
        return true;
    }

    bool list(const BackendAgentRecord& live,
              const std::string& backendId,
              std::int64_t now,
              std::vector<std::string>& ids,
              std::vector<std::string>& labels,
              bool* partial = nullptr,
              std::vector<std::string>* folders = nullptr,
              std::vector<std::string>* statuses = nullptr) const
    {
        ids.clear();
        labels.clear();
        if (folders) folders->clear();
        if (statuses) statuses->clear();
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
        if (statuses && !current.statuses.empty()) {
            for (const auto& id : ids) {
                const auto found = current.statuses.find(id);
                if (found == current.statuses.end()) {
                    ids.clear(); labels.clear(); statuses->clear();
                    if (folders) folders->clear();
                    return false;
                }
                statuses->push_back(found->second);
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

    // Read-only presentation of the last successfully authenticated Agent
    // observation. A transport outage or an expired lease must not erase the
    // directory browser. Never use these IDs to authorize a mutation:
    // requestPreview(), dispatch and deletion continue to require a live
    // matching Agent generation and their existing Core checks.
    bool listLastObserved(const std::string& backendId,
                          std::vector<std::string>& ids,
                          std::vector<std::string>& labels,
                          bool* partial = nullptr,
                          std::vector<std::string>* folders = nullptr,
                          std::vector<std::string>* statuses = nullptr) const
    {
        ids.clear();
        labels.clear();
        if (folders) folders->clear();
        if (statuses) statuses->clear();
        if (partial) *partial = true;
        std::lock_guard<std::mutex> guard(mutex_);
        const auto found = snapshots_.find(backendId);
        if (found == snapshots_.end()) return false;
        const auto& snapshot = found->second;
        // publish() already validated and associated this bounded snapshot
        // with the authenticated Agent at capture time.
        ids = snapshot.ids;
        if (partial) *partial = snapshot.partial;
        if (!snapshot.labels.empty()) {
            for (const auto& id : ids) {
                const auto label = snapshot.labels.find(id);
                if (label == snapshot.labels.end()) return false;
                labels.push_back(label->second);
            }
        }
        if (folders && !snapshot.folders.empty()) {
            for (const auto& id : ids) {
                const auto folder = snapshot.folders.find(id);
                if (folder == snapshot.folders.end()) return false;
                folders->push_back(folder->second);
            }
        }
        if (statuses && !snapshot.statuses.empty()) {
            for (const auto& id : ids) {
                const auto status = snapshot.statuses.find(id);
                if (status == snapshot.statuses.end()) return false;
                statuses->push_back(status->second);
            }
        }
        return true;
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
        std::map<std::string, std::string> statuses;
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
    // Persist only successes keyed by the scanned file fingerprint. Arbitrary
    // manually configured source IDs are not proof of unchanged file bytes.
    static bool persistentId(const std::string& id)
    {
        return id.size() == 37 && id.compare(0, 5, "scan_") == 0 &&
            std::all_of(id.begin() + 5, id.end(), [](unsigned char c) {
                return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
            });
    }

    static std::string pack(const Preview& p)
    {
        const std::array<std::string, 16> fields{{
            p.status, p.title, p.targetFolder, p.videoCodec,
            std::to_string(p.sourceBytes), std::to_string(p.availableBytes),
            std::to_string(p.requiredBytes), p.videoReencodeRequired ? "1" : "0",
            p.failureCode, p.nfoOutline, p.nfoPlot, p.nfoYear, p.nfoGenre,
            std::to_string(p.durationSeconds), p.posterDataUri, p.fanartDataUri
        }};
        std::string encoded;
        for (const auto& field : fields) {
            encoded += std::to_string(field.size());
            encoded += ':';
            encoded += field;
        }
        return encoded;
    }

    static bool unpack(const std::string& encoded, Preview& p)
    {
        std::array<std::string, 16> fields;
        std::size_t offset = 0;
        for (auto& field : fields) {
            const auto colon = encoded.find(':', offset);
            if (colon == std::string::npos || colon == offset ||
                colon - offset > 8) return false;
            std::size_t length = 0;
            for (std::size_t i = offset; i < colon; ++i) {
                if (encoded[i] < '0' || encoded[i] > '9') return false;
                length = length * 10 + (encoded[i] - '0');
            }
            offset = colon + 1;
            if (length > encoded.size() - offset) return false;
            field.assign(encoded, offset, length);
            offset += length;
        }
        if (offset != encoded.size() || fields[0] != "ready" ||
            fields[7] != "0" && fields[7] != "1") return false;
        const auto number = [](const std::string& v, std::uint64_t& out) {
            if (v.empty() || v.size() > 20) return false;
            std::uint64_t n = 0;
            for (char c : v) {
                if (c < '0' || c > '9' ||
                    n > (std::numeric_limits<std::uint64_t>::max() -
                         (c - '0')) / 10) return false;
                n = n * 10 + (c - '0');
            }
            out = n;
            return true;
        };
        if (!number(fields[4], p.sourceBytes) ||
            !number(fields[5], p.availableBytes) ||
            !number(fields[6], p.requiredBytes) ||
            !number(fields[13], p.durationSeconds)) return false;
        p.status = fields[0];
        p.title = fields[1];
        p.targetFolder = fields[2];
        p.videoCodec = fields[3];
        p.videoReencodeRequired = fields[7] == "1";
        p.failureCode = fields[8];
        p.nfoOutline = fields[9];
        p.nfoPlot = fields[10];
        p.nfoYear = fields[11];
        p.nfoGenre = fields[12];
        p.posterDataUri = fields[14];
        p.fanartDataUri = fields[15];
        return p.title.size() <= 120 && !p.title.empty() &&
            p.videoCodec.size() <= 64 && !p.videoCodec.empty() &&
            p.targetFolder.size() <= 512 && !p.targetFolder.empty();
    }

    bool persistentPreview(const std::string& backend, const std::string& id,
                           Preview& result)
    {
        if (!persistent_ || !persistentId(id)) return false;
        auto lease = database_->acquireTransactionLease();
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(database_->handle(),
            "SELECT payload FROM media_import_checked_previews "
            "WHERE backend_id=? AND source_id=?", -1, &statement, nullptr)
            != SQLITE_OK) return false;
        sqlite3_bind_text(statement, 1, backend.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 2, id.c_str(), -1, SQLITE_TRANSIENT);
        bool found = false;
        if (sqlite3_step(statement) == SQLITE_ROW) {
            const auto* blob = sqlite3_column_blob(statement, 0);
            const int size = sqlite3_column_bytes(statement, 0);
            if (blob && size > 0 && size <= 1000000) {
                Preview decoded;
                decoded.sourceId = id;
                if (unpack(std::string(static_cast<const char*>(blob),
                                       static_cast<std::size_t>(size)), decoded)) {
                    result = std::move(decoded);
                    found = true;
                }
            }
        }
        sqlite3_finalize(statement);
        return found;
    }

    void rememberPreview(const std::string& backend, const Preview& preview)
    {
        if (!persistent_ || !persistentId(preview.sourceId)) return;
        const std::string packed = pack(preview);
        if (packed.size() > 1000000) return;
        auto lease = database_->acquireTransactionLease();
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(database_->handle(),
            "INSERT OR REPLACE INTO media_import_checked_previews "
            "(backend_id,source_id,payload) VALUES (?,?,?)", -1,
            &statement, nullptr) != SQLITE_OK) return;
        sqlite3_bind_text(statement, 1, backend.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 2, preview.sourceId.c_str(), -1,
                          SQLITE_TRANSIENT);
        sqlite3_bind_blob(statement, 3, packed.data(),
                          static_cast<int>(packed.size()), SQLITE_TRANSIENT);
        sqlite3_step(statement);
        sqlite3_finalize(statement);
    }

    Database* database_ = nullptr;
    bool persistent_ = false;
    mutable std::mutex mutex_;
    std::map<std::string, Snapshot> snapshots_;
    std::map<std::string, PreviewSlot> previews_;
    // A cached preview is advisory and bounded in age. Free space and
    // transient probe failures can change without changing source identity;
    // a deliberate selection rechecks those after 60 seconds.
    std::map<std::string, std::map<std::string,
        std::pair<std::int64_t, Preview>>> checked_;
    std::map<std::string, std::set<std::string>> attempted_;
};
