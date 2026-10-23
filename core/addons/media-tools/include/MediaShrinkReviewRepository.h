#pragma once

#include <cstdint>
#include <string>

class Database;
namespace vdrsuite::operations { class MutationOperationRepository; }

namespace vdrsuite::addons {

// A review binding in the existing Suite Core SQLite database. This is NOT
// a command queue, a filesystem pathname registry, or execution authority.
// Only completed canonical stage operations can create review bindings.
enum class MediaShrinkReviewStatus {
    ok, replayed, invalid, notFound, conflict, unavailable
};

struct MediaShrinkReview {
    std::string reviewId;          // shk_ + 32 lowercase hex chars
    std::string stageOperationId;  // canonical ADR-0042 operation
    std::string publishOperationId;
    std::string actorId;
    std::string backendId;
    std::string recordingId;       // canonical rec_ + 32 lowercase hex chars
    std::uint64_t backendGeneration = 0;
    std::string sourceFingerprint;
    std::string state;             // staged | publishing | published
    std::uint64_t revision = 0;
    std::string publishedRecordingId;
};

struct MediaShrinkReviewResult {
    MediaShrinkReviewStatus status = MediaShrinkReviewStatus::unavailable;
    MediaShrinkReview review;
    bool ok() const { return status == MediaShrinkReviewStatus::ok; }
};

class MediaShrinkReviewRepository final {
public:
    MediaShrinkReviewRepository(
        Database& database,
        vdrsuite::operations::MutationOperationRepository& operations);
    bool ensureSchema();

    // Exact successful stage Operation required. Its opaque resultReference
    // must equal reviewId; no path or worker-selected authorization is stored.
    MediaShrinkReviewResult registerStaged(
        const std::string& reviewId,
        const std::string& stageOperationId);

    // Actor-/backend-scoped read; no global or cross-actor enumeration.
    MediaShrinkReviewResult find(
        const std::string& actorId, const std::string& backendId,
        const std::string& reviewId) const;

    // Separately reserved canonical publication Operation required. Once a
    // review is claimed, even an unknown Agent outcome never enables retries.
    MediaShrinkReviewResult claimPublication(
        const std::string& actorId, const std::string& backendId,
        const std::string& reviewId, std::uint64_t expectedRevision,
        const std::string& publishOperationId);

    // Succeeds only after the canonical publish Operation is reconciled and
    // its resultReference is an opaque recording ID. No filesystem mutation.
    MediaShrinkReviewResult completePublication(
        const std::string& actorId, const std::string& backendId,
        const std::string& reviewId, std::uint64_t expectedRevision);

private:
    Database& database_;
    vdrsuite::operations::MutationOperationRepository& operations_;
};
} // namespace vdrsuite::addons
