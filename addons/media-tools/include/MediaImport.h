#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace vdrsuite::mediatools {
namespace fs = std::filesystem;

// This component is not a public or actor-facing API. Only an authorized
// backend-local service may provide these roots in a later integration.
struct ImportRequest {
    fs::path sourceRoot;
    fs::path videoRoot;
    fs::path sourceFile;
};

struct ImportPlan {
    fs::path sourceRoot;
    fs::path videoRoot;
    fs::path sourceFile;
    fs::path titleDirectory;
    std::string title;
    std::uintmax_t sourceBytes = 0;
    std::uint64_t sourceDevice = 0;
    std::uint64_t sourceInode = 0;
    std::int64_t sourceMtimeSeconds = 0;
    std::int64_t sourceMtimeNanoseconds = 0;
    bool duplicateTarget = false; // advisory only; normal import always rejects duplicates
};

struct ProcessResult {
    int exitCode = -1;
    std::string output;
};

// No shell interpolation: implementation uses execv with fixed executable paths.
struct ProcessRunner {
    virtual ~ProcessRunner() = default;
    virtual ProcessResult run(const std::vector<std::string>& argv, bool captureOutput) = 0;
};

class NativeProcessRunner final : public ProcessRunner {
public:
    explicit NativeProcessRunner(
        std::chrono::seconds maximum = std::chrono::seconds(540));
    ProcessResult run(const std::vector<std::string>& argv, bool captureOutput) override;
    // Used only by the backend Agent's scoped Media Tools maintenance guard.
    // -1 clears the binding. The Runner never owns or closes the descriptor.
    void bindMaintenanceLockFd(int fd) noexcept { maintenanceLockFd_ = fd; }
private:
    std::chrono::seconds maximum_;
    int maintenanceLockFd_ = -1;
};

// FFprobe on MPEG-TS can repeat one stream under stream/program entries.
// Accept only one consistent, strictly checked video codec token, even if
// repeated; never take the first value if later entries disagree.
std::string parseProbedVideoCodec(const std::string& text);

ImportPlan inspectImport(const ImportRequest& request);
// Preview-only observation: allow identifying an existing target recording
// without accidentally granting write authority or creating directories.
struct ImportPreview {
    ImportPlan plan;
    std::string videoCodec;
    bool videoReencodeRequired = false;
    bool duplicate = false;
    // Capacity is reported as a separate observation below.
};
ImportPreview inspectImportPreview(const ImportRequest& request,
                                   ProcessRunner& runner);
// Identical path/source validation, but returns duplicate status for a
// read-only browser preview instead of rejecting it as a write attempt.
ImportPlan inspectImportPreview(const ImportRequest& request);
// Read-only reinspection of an explicitly selected .duplicate source.
// Reconstruct the pre-marker title and require a matching complete VDR recording.
ImportPlan inspectMarkedDuplicatePreview(const ImportRequest& request);

// The existing bounded adjacent .nfo parser, reused for read-only display.
struct ImportLocalMetadata {
    std::string title, shortText, description, year, genre;
};
ImportLocalMetadata readImportLocalMetadata(const ImportPlan& plan);

// MT.META6: typed, already-confirmed provider selection. Only a trusted
// Core/Agent import binding may supply this value; normal imports continue
// to use adjacent NFO without behavioral change. This is NOT an API payload
// or an authorization to locate or mutate an arbitrary existing recording.
struct ImportConfirmedMetadata {
    std::string mediaType; // "episode" or "movie"
    std::string title; // episode or movie title
    std::string seriesTitle; // required presentation title for episodes
    std::string overview;
    // Optional identity from the same pre-import TMDb response (v2 only).
    // v1 remains fully readable, with these fields left empty.
    std::string externalNamespace;
    std::string externalId;
    std::string posterReference;
    int seasonNumber = 0;
    int episodeNumber = 0;
};

// Shared, header-only decoder: the daemon can read the SAME immutable
// metadata bytes after a browser exits, without linking the media worker,
// adding a database, or querying TMDb.
inline bool parseFrozenImportMetadata(
    const std::string& snapshot, ImportConfirmedMetadata& output)
{
    output = {};
    if (snapshot.size() < 12U || snapshot.size() > 4096U) return false;
    const bool version1 = snapshot.compare(0, 3, "v1|") == 0;
    const bool version2 = snapshot.compare(0, 3, "v2|") == 0;
    if (!version1 && !version2) return false;
    std::array<std::string, 10> parts{};
    const std::size_t count = version2 ? 10U : 7U;
    std::size_t from = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const auto end = snapshot.find('|', from);
        if ((i + 1 < count && end == std::string::npos) ||
            (i + 1 == count && end != std::string::npos)) return false;
        parts[i] = snapshot.substr(from,
            (end == std::string::npos ? snapshot.size() : end) - from);
        from = end == std::string::npos ? snapshot.size() : end + 1;
    }
    if ((parts[1] != "episode" && parts[1] != "movie")) return false;
    const auto decode = [](const std::string& input, std::string& value) {
        const auto nibble = [](unsigned char ch) -> int {
            if (ch >= '0' && ch <= '9') return ch - '0';
            if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
            if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
            return -1;
        };
        value.clear();
        for (std::size_t i = 0; i < input.size(); ++i) {
            unsigned char ch = static_cast<unsigned char>(input[i]);
            if (ch == '%') {
                if (i + 2 >= input.size()) return false;
                const int high = nibble(static_cast<unsigned char>(input[++i]));
                const int low = nibble(static_cast<unsigned char>(input[++i]));
                if (high < 0 || low < 0) return false;
                ch = static_cast<unsigned char>((high << 4) | low);
            } else if (!(std::isalnum(ch) || ch == '_' || ch == '.' ||
                         ch == '~' || ch == '!' || ch == '*' ||
                         ch == 39 || ch == '(' || ch == ')' || ch == '-'))
                return false;
            if (ch == 0) return false;
            value.push_back(static_cast<char>(ch));
        }
        return true;
    };
    const auto number = [](const std::string& input, int& result) {
        if (input.empty() || input.size() > 5 ||
            !std::all_of(input.begin(), input.end(), [](unsigned char ch) {
                return ch >= '0' && ch <= '9';
            })) return false;
        try { result = std::stoi(input); }
        catch (...) { return false; }
        return result >= 0;
    };
    if (!decode(parts[2], output.seriesTitle) ||
        !decode(parts[3], output.title) ||
        !decode(parts[4], output.overview) ||
        !number(parts[5], output.seasonNumber) ||
        !number(parts[6], output.episodeNumber) ||
        output.title.empty()) return false;
    output.mediaType = parts[1];
    if (version2) {
        // The external TMDb identity is already part of the candidate that
        // produced this snapshot. No provider request is made by the worker.
        output.externalNamespace = parts[7];
        output.externalId = parts[8];
        if (!decode(parts[9], output.posterReference)) return false;
        const bool numeric = !output.externalId.empty() &&
            output.externalId.size() <= 16U &&
            std::all_of(output.externalId.begin(), output.externalId.end(),
                [](unsigned char ch) { return ch >= '0' && ch <= '9'; });
        if (!numeric ||
            output.externalNamespace !=
                (output.mediaType == "episode" ? "tv-episode" : "movie") ||
            (output.posterReference.size() > 255U) ||
            (!output.posterReference.empty() &&
             (output.posterReference.front() != '/' ||
              output.posterReference.find("..") != std::string::npos)))
            return false;
    }
    if (output.mediaType == "episode")
        return !output.seriesTitle.empty() && output.episodeNumber > 0;
    return output.seriesTitle.empty() && output.seasonNumber == 0 &&
        output.episodeNumber == 0;
}


// Pure writer; no VDR filesystem access. The verified provider selection
// takes priority for present fields; local NFO supplies missing fields.
std::string renderNativeVdrRecordingInfo(
    const ImportPlan& plan, const ImportLocalMetadata& local,
    const ImportConfirmedMetadata* confirmed = nullptr);

// MT.META6.3 pure transform for an already imported Recording. Preserve
// all VDR fields other than T/S/D, their order and their values. No file IO
// or mutation authority is provided by this helper.
std::string mergeConfirmedMetadataIntoNativeVdrInfo(
    const std::string& originalInfo, const ImportConfirmedMetadata& confirmed);
struct ImportLocalArtwork { std::string posterDataUri, fanartDataUri; };
ImportLocalArtwork readImportLocalArtwork(const ImportPlan& plan);
// Optional FFprobe duration, never import authority.
double probeImportDurationSeconds(const ImportPlan& plan, ProcessRunner& runner);

// Stronger read-only evidence than duplicateTarget: a structurally complete
// VDR recording with matching info title, index and first TS segment. An
// incomplete .rec still prevents overwriting, but must not be auto-marked.
bool confirmedMediaImportDuplicate(const ImportPlan& plan);

// Read-only source classification for an explicit local/desktop/skip decision.
// This is informative only; execution probes the pinned source again.
// An unknown/unsupported video codec never counts as copy-compatible.
std::string probeImportVideoCodec(const ImportPlan& plan, ProcessRunner& runner);
bool importVideoNeedsReencode(const std::string& codec);

// Import is intentionally a separate call. Preflight is supplied by the
// backend-local caller; the CLI always passes the hard reserve policy.
// Video re-encoding is opt-in. Agent callers get fail-closed by default;
// a separately authenticated Core decision is required before setting true.
fs::path executeRemuxImport(const ImportPlan& plan, ProcessRunner& runner,
                            const std::function<void(const ImportPlan&)>& capacityPreflight,
                            bool allowVideoReencode = false,
                            const ImportConfirmedMetadata* confirmedMetadata = nullptr);

// Read-only capacity calculation using the exact same budget as production
// admission. A preview can report insufficient space without staging files.
struct ImportCapacityPreview {
    std::uint64_t availableBytes = 0;
    std::uint64_t requiredBytes = 0;
    std::uint64_t reserveBytes = 0;
    bool writable = false;
    bool sufficient = false;
};
ImportCapacityPreview inspectImportCapacity(const ImportPlan& plan);

void requireProductionCapacity(const ImportPlan& plan);

} // namespace vdrsuite::mediatools
