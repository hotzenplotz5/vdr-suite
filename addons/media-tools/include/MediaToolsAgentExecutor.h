#pragma once

#include "BackendAgentCommand.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::mediatools {

inline constexpr const char* kMediaImportCommandType = "addon.media.import";

// Only a backend-local opaque inventory ID. Never deserialize a file path.
inline bool parseMediaImportSelection(const std::string& payload, std::string& sourceId)
{
    sourceId.clear();
    const std::string prefix = "{\"sourceId\":\"";
    if (payload.size() < prefix.size() + 3 ||
        payload.compare(0, prefix.size(), prefix) != 0 ||
        payload.substr(payload.size() - 2) != "\"}")
        return false;
    sourceId = payload.substr(prefix.size(), payload.size() - prefix.size() - 2);
    if (sourceId.empty() || sourceId.size() > 64 ||
        !std::all_of(sourceId.begin(), sourceId.end(), [](unsigned char ch) {
            return std::isalnum(ch) || ch == '_' || ch == '-';
        }))
    {
        sourceId.clear();
        return false;
    }
    return true;
}

// The second, explicitly selected v1 API decision has its own command-payload
// version. Never infer local video-encoding approval from the ordinary import
// grant or from the legacy sourceId-only payload.
inline bool parseMediaImportSelection(const std::string& payload,
                                     std::string& sourceId,
                                     bool& approveVideoReencode)
{
    approveVideoReencode = false;
    if (parseMediaImportSelection(payload, sourceId)) return true;
    sourceId.clear();
    const std::string prefix = "{\"sourceId\":\"";
    const std::string suffix = "\",\"videoDecision\":\"local\"}";
    if (payload.size() < prefix.size() + suffix.size() + 16U ||
        payload.compare(0, prefix.size(), prefix) != 0 ||
        payload.compare(payload.size() - suffix.size(),
                        suffix.size(), suffix) != 0)
        return false;
    sourceId = payload.substr(prefix.size(),
        payload.size() - prefix.size() - suffix.size());
    if (sourceId.size() > 64U ||
        !std::all_of(sourceId.begin(), sourceId.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '_' || c == '-';
        })) {
        sourceId.clear();
        return false;
    }
    approveVideoReencode = true;
    return true;
}

// Reuse the one Media Import command; versions 3/4 represent a verified
// backend-local source marker, NOT a VDR recording.
enum class MediaImportDecision { import, local, desktop, skip, release };
inline bool parseMediaImportSelection(
    const std::string& payload, std::string& sourceId,
    MediaImportDecision& decision)
{
    decision = MediaImportDecision::import;
    bool encode = false;
    if (parseMediaImportSelection(payload, sourceId, encode)) {
        decision = encode ? MediaImportDecision::local
                          : MediaImportDecision::import;
        return true;
    }
    const std::string prefix = "{\"sourceId\":\"";
    for (const auto& option : {
        std::pair<const char*, MediaImportDecision>{"desktop", MediaImportDecision::desktop},
        {"skip", MediaImportDecision::skip},
        {"release", MediaImportDecision::release}
    }) {
        const std::string suffix =
            std::string("\",\"videoDecision\":\"") + option.first + "\"}";
        if (payload.size() < prefix.size() + suffix.size() + 16 ||
            payload.compare(0, prefix.size(), prefix) != 0 ||
            payload.compare(payload.size() - suffix.size(),
                            suffix.size(), suffix) != 0)
            continue;
        const std::string id = payload.substr(
            prefix.size(), payload.size() - prefix.size() - suffix.size());
        if (id.size() > 64 ||
            !std::all_of(id.begin(), id.end(),
                [](unsigned char ch) {
                    return std::isalnum(ch) || ch == '_' || ch == '-';
                })) return false;
        sourceId = id;
        decision = option.second;
        return true;
    }
    return false;
}
inline unsigned mediaImportSelectionVersion(MediaImportDecision decision)
{
    switch (decision) {
        case MediaImportDecision::import: return 1U;
        case MediaImportDecision::local: return 2U;
        case MediaImportDecision::desktop: return 3U;
        case MediaImportDecision::skip: return 4U;
        case MediaImportDecision::release: return 5U;
    }
    return 0U;
}
inline std::string mediaImportSelectionJson(
    const std::string& sourceId, MediaImportDecision choice)
{
    const char* option = "";
    switch (choice) {
        case MediaImportDecision::import:
            return "{\"sourceId\":\"" + sourceId + "\"}";
        case MediaImportDecision::local: option = "local"; break;
        case MediaImportDecision::desktop: option = "desktop"; break;
        case MediaImportDecision::skip: option = "skip"; break;
        case MediaImportDecision::release: option = "release"; break;
    }
    return "{\"sourceId\":\"" + sourceId +
        "\",\"videoDecision\":\"" + option + "\"}";
}
inline std::string mediaImportMarkerEvidence(
    const std::string& sourceId, MediaImportDecision choice)
{
    if (choice != MediaImportDecision::desktop &&
        choice != MediaImportDecision::skip &&
        choice != MediaImportDecision::release) return {};
    return std::string("media-tools-mark:") +
        (choice == MediaImportDecision::desktop ? "desktop:" :
         choice == MediaImportDecision::skip ? "skip:" : "release:") +
        sourceId;
}

inline unsigned mediaImportSelectionVersion(bool approveVideoReencode)
{
    return approveVideoReencode ? 2U : 1U;
}

// Safe display metadata is strictly advisory, never a native source path.
struct MediaImportSourceLabel {
    std::string sourceId;
    std::string label;
    // Read-only, root-relative folder presentation. Never a path authority.
    std::string folder = {};
};

// Read-only observation from the fixed root-admin approval + pinned binary.
// It never authorizes execution or substitutes for a live Core grant.
struct MediaToolsInstallationEvidence {
    std::string origin;
    std::string version;
    std::string sha256;
};

enum class MediaImportAgentOutcome {
    rejected,  // Proven no effect
    submitted, // Handler accepted; recording still needs reconciliation
    marked,    // Verified source marker; no VDR recording created
    unknown    // Ambiguous effect; never retry blindly
};

// Add-on implementation lives here. Core owns durable receipts and results.
// Only trusted Agent composition may inject this interface; production does
// not currently inject it, so neither config nor manifest can activate it.
// authorize() rechecks backend grant, pinned approval and local inventory.
class IMediaImportAgentExecutor {
public:
    virtual ~IMediaImportAgentExecutor() = default;
    virtual bool available() = 0;
    virtual bool verifiedInstallation(MediaToolsInstallationEvidence& out)
    { out = {}; return false; }
    // Default deny; only explicit trusted Agent composition may enumerate.
    virtual bool listTrustedSourceIds(std::vector<std::string>& out)
    { out.clear(); return false; }
    // Source labels are not authorization tokens and cannot select paths.
    virtual bool listTrustedSourceLabels(std::vector<MediaImportSourceLabel>& out)
    { out.clear(); return false; }
    // A single bounded inventory observation. Existing test executors can
    // use the conservative default (partial/unknown); production overrides
    // this to read IDs, display names and completion from ONE scan.
    virtual bool listTrustedSourceSnapshot(
        std::vector<std::string>& ids,
        std::vector<MediaImportSourceLabel>& labels, bool& partial)
    {
        ids.clear();
        labels.clear();
        partial = true;
        if (!listTrustedSourceIds(ids)) return false;
        if (!listTrustedSourceLabels(labels) || labels.size() != ids.size())
            labels.clear();
        return true;
    }
    virtual bool authorize(const BackendAgentCommandAssignment& assignment,
                           const std::string& backendId,
                           std::uint64_t generation) = 0;
    virtual MediaImportAgentOutcome execute(
        const BackendAgentCommandAssignment& assignment) = 0;
    // One-shot, empty unless the reviewed handler reported a committed
    // recording. Never a reason to mark a VDR recording verified by itself.
    virtual std::string takeRecordingEvidence() { return {}; }
    virtual std::string takeMarkerEvidence() { return {}; }
};

} // namespace vdrsuite::mediatools
