#pragma once

#include "BackendAgentCommand.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
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
    virtual bool authorize(const BackendAgentCommandAssignment& assignment,
                           const std::string& backendId,
                           std::uint64_t generation) = 0;
    virtual MediaImportAgentOutcome execute(
        const BackendAgentCommandAssignment& assignment) = 0;
    // One-shot, empty unless the reviewed handler reported a committed
    // recording. Never a reason to mark a VDR recording verified by itself.
    virtual std::string takeRecordingEvidence() { return {}; }
};

} // namespace vdrsuite::mediatools
