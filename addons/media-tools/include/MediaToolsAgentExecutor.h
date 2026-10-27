#pragma once

#include "BackendAgentCommand.h"

#include "MediaImportRecordingEvidence.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::mediatools {

inline constexpr const char* kMediaImportCommandType = "addon.media.import";
// Readback notification uses the existing durable Agent command journal.
// Core's confirmed Recording-cache readback is the only cleanup trigger.
// The Agent verifies the original source snapshot again before mutation.
inline constexpr const char* kMediaImportReadbackCommandType =
    "addon.media.import.readback";

inline std::string mediaImportReadbackPayload(
    const std::string& sourceId, const std::string& originalCommandId,
    const std::string& recordingEvidence,
    const std::string& sourceEvidence = {})
{
    std::string payload = "{\"sourceId\":\"" + sourceId +
        "\",\"originalCommandId\":\"" + originalCommandId +
        "\",\"recordingEvidence\":\"" + recordingEvidence + "\"";
    if (!sourceEvidence.empty())
        payload += ",\"sourceIdentity\":\"" + sourceEvidence + "\"";
    return payload + "}";
}

inline bool parseMediaImportReadbackPayload(
    const std::string& payload, std::string& sourceId,
    std::string& originalCommandId, std::string& recordingEvidence,
    std::string& sourceEvidence)
{
    sourceId.clear();
    originalCommandId.clear();
    recordingEvidence.clear();
    sourceEvidence.clear();
    constexpr const char* start = "{\"sourceId\":\"";
    constexpr const char* middle = "\",\"originalCommandId\":\"";
    constexpr const char* end = "\",\"recordingEvidence\":\"";
    if (payload.compare(0, std::char_traits<char>::length(start), start) != 0)
        return false;
    const auto first = payload.find(middle, std::char_traits<char>::length(start));
    if (first == std::string::npos) return false;
    const auto second = payload.find(end, first + std::char_traits<char>::length(middle));
    if (second == std::string::npos ||
        payload.size() < second + std::char_traits<char>::length(end) + 3 ||
        payload.back() != '}') return false;
    sourceId = payload.substr(std::char_traits<char>::length(start),
        first - std::char_traits<char>::length(start));
    originalCommandId = payload.substr(
        first + std::char_traits<char>::length(middle),
        second - first - std::char_traits<char>::length(middle));
    const auto evidenceStart = second + std::char_traits<char>::length(end);
    const auto evidenceEnd = payload.find('"', evidenceStart);
    if (evidenceEnd == std::string::npos) return false;
    recordingEvidence = payload.substr(evidenceStart, evidenceEnd - evidenceStart);
    const auto suffix = payload.substr(evidenceEnd + 1);
    if (suffix != "}") {
        constexpr const char* sourcePrefix = ",\"sourceIdentity\":\"";
        const std::string prefix(sourcePrefix);
        if (suffix.size() < prefix.size() + 2 ||
            suffix.compare(0, prefix.size(), prefix) != 0 ||
            suffix.substr(suffix.size() - 2) != "\"}") return false;
        sourceEvidence = suffix.substr(prefix.size(),
            suffix.size() - prefix.size() - 2);
        if (!validMediaImportSourceEvidence(sourceEvidence)) return false;
    }
    const auto opaque = [](const std::string& value, std::size_t maximum) {
        return !value.empty() && value.size() <= maximum &&
            std::all_of(value.begin(), value.end(), [](unsigned char ch) {
                return std::isalnum(ch) || ch == '_' || ch == '-';
            });
    };
    return opaque(sourceId, 64) && sourceId.size() >= 16 &&
        opaque(originalCommandId, 128) &&
        validRecordingPathEvidence(recordingEvidence) &&
        payload == mediaImportReadbackPayload(
            sourceId, originalCommandId, recordingEvidence, sourceEvidence);
}

inline bool parseMediaImportReadbackPayload(
    const std::string& payload, std::string& sourceId,
    std::string& originalCommandId, std::string& recordingEvidence)
{
    std::string sourceEvidence;
    return parseMediaImportReadbackPayload(
        payload, sourceId, originalCommandId, recordingEvidence,
        sourceEvidence);
}

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
enum class MediaImportDecision { import, local, desktop, skip, release, duplicate };
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
        {"release", MediaImportDecision::release},
        {"duplicate", MediaImportDecision::duplicate}
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
        case MediaImportDecision::duplicate: return 6U;
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
        case MediaImportDecision::duplicate: option = "duplicate"; break;
    }
    return "{\"sourceId\":\"" + sourceId +
        "\",\"videoDecision\":\"" + option + "\"}";
}
inline std::string mediaImportMarkerEvidence(
    const std::string& sourceId, MediaImportDecision choice)
{
    if (choice != MediaImportDecision::desktop &&
        choice != MediaImportDecision::skip &&
        choice != MediaImportDecision::duplicate &&
        choice != MediaImportDecision::release) return {};
    return std::string("media-tools-mark:") +
        (choice == MediaImportDecision::desktop ? "desktop:" :
         choice == MediaImportDecision::skip ? "skip:" :
         choice == MediaImportDecision::duplicate ? "duplicate:" : "release:") +
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

// Ephemeral observation for ONE opaque source ID selected by an actor.
// No absolute backend path crosses this boundary. A normal import rechecks
// all facts at execution, independently of this read-only preview.
struct MediaImportSourcePreview {
    std::string sourceId;
    std::string status = "unavailable"; // ready, duplicate, insufficient, unavailable
    std::string title;
    std::string targetFolder; // VDR-video-root relative, not a path authority
    std::string videoCodec;
    std::uint64_t sourceBytes = 0;
    std::uint64_t availableBytes = 0;
    std::uint64_t requiredBytes = 0;
    bool videoReencodeRequired = false;
    std::string failureCode; // read-only stage; never a source path or process log
    std::string nfoOutline, nfoPlot, nfoYear, nfoGenre;
    std::uint64_t durationSeconds = 0;
};

// Read-only observation from the fixed root-admin approval + pinned binary.
// It never authorizes execution or substitutes for a live Core grant.
struct MediaToolsInstallationEvidence {
    std::string origin;
    std::string version;
    std::string sha256;
};

enum class MediaImportCleanupOutcome {
    removed, alreadyAbsent, notPerformed, failed
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
    virtual bool inspectTrustedSource(
        const std::string& sourceId, MediaImportSourcePreview& out)
    { out = {}; (void)sourceId; return false; }
    virtual bool authorize(const BackendAgentCommandAssignment& assignment,
                           const std::string& backendId,
                           std::uint64_t generation) = 0;
    virtual MediaImportAgentOutcome execute(
        const BackendAgentCommandAssignment& assignment) = 0;
    // One-shot, empty unless the reviewed handler reported a committed
    // recording. Never a reason to mark a VDR recording verified by itself.
    virtual std::string takeRecordingEvidence() { return {}; }
    virtual std::string takeMarkerEvidence() { return {}; }
    // A confirmed Core readback may cleanup the same backend-local source.
    // Default fails closed; no browser-supplied filesystem paths.
    virtual MediaImportCleanupOutcome cleanupConfirmedSource(
        const std::string&, const std::string&)
    { return MediaImportCleanupOutcome::notPerformed; }
};

} // namespace vdrsuite::mediatools
