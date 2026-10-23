#pragma once

#include "DaemonMediaShrinkPlanRuntime.h"
#include "MediaShrinkOperationAdmission.h"
#include "MediaShrinkOperationFence.h"
#include "MediaShrinkReviewRepository.h"
#include "MediaShrinkAgentCommand.h"

#include <cstdint>
#include <string>

// A fail-closed Core-only binding between canonical PublicRecordingIdentity,
// current VDR snapshot, reviewed mutation grant, backend lease and immutable
// ADR-0042 reservation. No client path or Agent-local review path is accepted.
class DaemonMediaShrinkAdmissionAuthority final
    : public vdrsuite::addons::IMediaShrinkAdmissionAuthority {
public:
    DaemonMediaShrinkAdmissionAuthority(
        const DaemonMediaShrinkPlanRuntime& sources,
        const vdrsuite::addons::MediaShrinkOperationFence& fence,
        vdrsuite::addons::MediaShrinkReviewRepository& reviews)
        : sources_(sources), fence_(fence), reviews_(reviews) {}

    bool allowStage(const std::string& actor, const std::string& backend,
                    const std::string& recordingId,
                    const std::string& sourceFingerprint, std::int64_t now,
                    std::uint64_t& generation) override {
        generation = 0;
        const auto current =
            sources_.trustedSourceFingerprint(backend, recordingId);
        if (!current || *current != sourceFingerprint) return false;
        std::string reason;
        return fence_.effectiveCapability(
            actor, backend,
            vdrsuite::mediatools::kMediaShrinkStageCommandType,
            now, generation, reason);
    }

    bool allowPublish(const std::string& actor, const std::string& backend,
                      const std::string& reviewId,
                      const std::string& sourceFingerprint, std::int64_t now,
                      std::uint64_t& generation) override {
        generation = 0;
        const auto review = reviews_.find(actor, backend, reviewId);
        if (!review.ok() ||
            (review.review.state != "staged" &&
             review.review.state != "publishing") ||
            review.review.sourceFingerprint != sourceFingerprint)
            return false;
        const auto current = sources_.trustedSourceFingerprint(
            backend, review.review.recordingId);
        if (!current || *current != sourceFingerprint) return false;
        std::string reason;
        return fence_.effectiveCapability(
            actor, backend,
            vdrsuite::mediatools::kMediaShrinkPublishCommandType,
            now, generation, reason);
    }
private:
    const DaemonMediaShrinkPlanRuntime& sources_;
    const vdrsuite::addons::MediaShrinkOperationFence& fence_;
    vdrsuite::addons::MediaShrinkReviewRepository& reviews_;
};
