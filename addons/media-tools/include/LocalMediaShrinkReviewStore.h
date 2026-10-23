#pragma once

#include "MediaShrink.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace vdrsuite::mediatools {

// Local, path-bearing Agent evidence. NEVER serialized onto the API or Core
// wire. The canonical MutationOperation and MediaShrinkReviewRepository still
// own authorization, review state and publication reconciliation.
struct LocalMediaShrinkReview {
    std::string reviewId;
    std::string actorId;
    std::string backendId;
    std::string recordingId;
    std::string stageOperationId;
    std::string sourceFingerprint;
    std::uint64_t backendGeneration = 0;
    fs::path videoRoot;
    fs::path relativeRecording;
    fs::path candidateDirectory;
};

// The directory is an explicitly provisioned, private, owner-only Agent
// state directory, NOT a user-selectable API or addon manifest path.
// All writes are atomic, no-replace, fsynced and bound to immutable source
// identity. A claimed review is never automatically retried after a crash.
class LocalMediaShrinkReviewStore final {
public:
    explicit LocalMediaShrinkReviewStore(std::filesystem::path privateDirectory);

    // Requires a successful, locally checked stageShrink result. Refuses to
    // overwrite an existing record, returning only an opaque review ID.
    std::string registerCandidate(const LocalMediaShrinkReview& evidence);

    // A read never authorizes execution. Callers must independently recheck
    // the live Core Operation, actor, backend grant, generation and revision.
    std::optional<LocalMediaShrinkReview> find(
        const std::string& reviewId) const;

    // Persist the irreversible start fence BEFORE publishShrinkCandidate.
    // A crash or uncertain result MUST be reconciled, never re-executed.
    bool claimOnce(const std::string& reviewId,
                   const std::string& publishOperationId);

private:
    std::filesystem::path directory_;
};
} // namespace vdrsuite::mediatools
