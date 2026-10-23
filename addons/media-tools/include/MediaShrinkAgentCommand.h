#pragma once

#include "MediaShrinkOperationSelection.h"
#include <cstdint>
#include <limits>
#include <regex>
#include <string>

namespace vdrsuite::mediatools {

inline constexpr const char* kMediaShrinkStageCommandType =
    "addon.media.shrink.stage";
inline constexpr const char* kMediaShrinkPublishCommandType =
    "addon.media.shrink.publish";

// Serialized by trusted Core only. These payloads contain no source/staging
// filenames, shell fragments, install paths or authority assertions.
struct MediaShrinkAgentSelection {
    ShrinkRequestAction action = ShrinkRequestAction::plan;
    std::string recordingId;
    std::string reviewId;
    std::string encoder = "software";
    unsigned maxHeight = 0;
    bool nightMode = false;
    std::uint64_t expectedReviewRevision = 0;
};

inline bool parseMediaShrinkAgentSelection(
    const std::string& commandType, const std::string& payload,
    MediaShrinkAgentSelection& output)
{
    output = {};
    if (payload.size() > 256) return false;
    if (commandType == kMediaShrinkStageCommandType) {
        static const std::regex stage(
            R"SH(^\{"recordingId":"(rec_[0-9a-f]{32})","encoder":"(software|vaapi|qsv|nvenc)","maxHeight":(0|[1-9][0-9]{0,3}),"nightMode":(true|false)\}$)SH");
        std::smatch match;
        if (!std::regex_match(payload, match, stage)) return false;
        output.action = ShrinkRequestAction::stage;
        output.recordingId = match[1].str();
        output.encoder = match[2].str();
        output.maxHeight = static_cast<unsigned>(std::stoul(match[3].str()));
        output.nightMode = match[4].str() == "true";
        const OpaqueShrinkSelection shape{
            output.recordingId, "", output.action, output.encoder,
            output.maxHeight, output.nightMode
        };
        return inspectOpaqueShrinkSelection(shape) ==
               ShrinkSelectionStatus::candidateOnly;
    }
    if (commandType == kMediaShrinkPublishCommandType) {
        static const std::regex publish(
            R"SH(^\{"reviewId":"(shk_[0-9a-f]{32})","revision":([1-9][0-9]{0,17})\}$)SH");
        std::smatch match;
        if (!std::regex_match(payload, match, publish)) return false;
        const auto revision = std::stoull(match[2].str());
        if (revision == 0 ||
            revision > static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max() - 1))
            return false;
        output.action = ShrinkRequestAction::publish;
        output.reviewId = match[1].str();
        output.expectedReviewRevision = revision;
        return true;
    }
    return false;
}

// No caller-selected path or permission. Core/Agent still need independent
// actor grants, package trust, immutable payload, live lease/generation,
// command receipt, local staging evidence and recording reconciliation.
} // namespace vdrsuite::mediatools
