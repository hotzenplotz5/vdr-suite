#pragma once

#include <cctype>
#include <string>

namespace vdrsuite::mediatools {

// Only the shape of a future API-v1/Core-owned HEVC request. No pathname,
// trusted authorization, package activation or filesystem action is conveyed.
enum class ShrinkRequestAction { plan, stage, publish };

struct OpaqueShrinkSelection {
    std::string recordingId;
    std::string candidateId;
    ShrinkRequestAction action = ShrinkRequestAction::plan;
    std::string encoder = "software";
    unsigned maxHeight = 0;
    bool nightMode = false;
};

enum class ShrinkSelectionStatus {
    candidateOnly, invalidRecordingId, invalidCandidateId, invalidOptions
};

inline bool shrinkOpaqueId(const std::string& value, const char* prefix) {
    const std::string head(prefix);
    if (value.size() != head.size() + 32 ||
        value.compare(0, head.size(), head) != 0)
        return false;
    for (std::size_t i = head.size(); i < value.size(); ++i)
        if (!((value[i] >= '0' && value[i] <= '9') ||
              (value[i] >= 'a' && value[i] <= 'f')))
            return false;
    return true;
}

inline ShrinkSelectionStatus inspectOpaqueShrinkSelection(
    const OpaqueShrinkSelection& request)
{
    if (request.action != ShrinkRequestAction::plan &&
        request.action != ShrinkRequestAction::stage &&
        request.action != ShrinkRequestAction::publish)
        return ShrinkSelectionStatus::invalidOptions;
    if (!shrinkOpaqueId(request.recordingId, "rec_"))
        return ShrinkSelectionStatus::invalidRecordingId;
    if (request.action == ShrinkRequestAction::publish) {
        if (!shrinkOpaqueId(request.candidateId, "shk_"))
            return ShrinkSelectionStatus::invalidCandidateId;
        if (request.encoder != "software" || request.maxHeight != 0 ||
            request.nightMode)
            return ShrinkSelectionStatus::invalidOptions;
    } else if (!request.candidateId.empty()) {
        return ShrinkSelectionStatus::invalidCandidateId;
    }
    if (request.action != ShrinkRequestAction::publish &&
        request.encoder != "software" && request.encoder != "vaapi" &&
        request.encoder != "qsv" && request.encoder != "nvenc")
        return ShrinkSelectionStatus::invalidOptions;
    if (request.maxHeight != 0 &&
        (request.maxHeight < 480 || request.maxHeight > 2160))
        return ShrinkSelectionStatus::invalidOptions;
    return ShrinkSelectionStatus::candidateOnly;
}

} // namespace vdrsuite::mediatools
