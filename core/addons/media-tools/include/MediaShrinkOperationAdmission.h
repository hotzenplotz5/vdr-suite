#pragma once

#include "MutationOperation.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations { class MutationOperationRepository; }
namespace vdrsuite::addons {
class MediaShrinkReviewRepository;

// The owning Core composition MUST implement this with live canonical
// identity, backend lease/generation, source snapshot and separate effective
// media.shrink.stage / media.shrink.publish grants. Default is no authority.
// Neither a route, addon manifest nor an Agent payload can implement it.
class IMediaShrinkAdmissionAuthority {
public:
    virtual ~IMediaShrinkAdmissionAuthority() = default;
    virtual bool allowStage(const std::string& actor,
                            const std::string& backend,
                            const std::string& recordingId,
                            const std::string& sourceFingerprint,
                            std::int64_t now,
                            std::uint64_t& backendGeneration) = 0;
    virtual bool allowPublish(const std::string& actor,
                              const std::string& backend,
                              const std::string& reviewId,
                              const std::string& sourceFingerprint,
                              std::int64_t now,
                              std::uint64_t& backendGeneration) = 0;
};

enum class MediaShrinkAdmissionStatus {
    accepted, replayed, invalid, forbidden, idempotencyConflict,
    reviewConflict, unavailable
};

struct MediaShrinkStageAdmission {
    std::string actorId;
    std::string backendId;
    std::string recordingId;
    std::string sourceFingerprint; // trusted current Core snapshot, not HTTP
    std::string idempotencyKey;
    std::string encoder = "software";
    unsigned maxHeight = 0;
    bool nightMode = false;
    std::int64_t now = 0;
};

struct MediaShrinkPublishAdmission {
    std::string actorId;
    std::string backendId;
    std::string reviewId;
    std::uint64_t expectedRevision = 0;
    std::string idempotencyKey;
    std::int64_t now = 0;
};

struct MediaShrinkAdmissionResult {
    MediaShrinkAdmissionStatus status = MediaShrinkAdmissionStatus::unavailable;
    vdrsuite::operations::MutationOperation operation;
};

// Reserves immutable Core operations. It performs NO FFmpeg work and
// dispatches NO Agent commands. Stage and publish are independently
// authorized, idempotent and bound to the same backend generation.
// The existing canonical assignment and verification owners MUST be used.
class MediaShrinkOperationAdmissionService final {
public:
    MediaShrinkOperationAdmissionService(
        vdrsuite::operations::MutationOperationRepository& operations,
        MediaShrinkReviewRepository& reviews,
        IMediaShrinkAdmissionAuthority& authority);

    MediaShrinkAdmissionResult stage(const MediaShrinkStageAdmission& request);
    MediaShrinkAdmissionResult publish(const MediaShrinkPublishAdmission& request);
private:
    vdrsuite::operations::MutationOperationRepository& operations_;
    MediaShrinkReviewRepository& reviews_;
    IMediaShrinkAdmissionAuthority& authority_;
};
} // namespace vdrsuite::addons
