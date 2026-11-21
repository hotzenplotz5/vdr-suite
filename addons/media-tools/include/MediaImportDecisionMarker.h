#pragma once

#include "PinnedMediaImportAgentExecutor.h"

namespace vdrsuite::mediatools {

// The caller must supply a source resolved by the trusted, backend-local
// inventory AFTER canonical Core/Agent authorization. Never accept a browser
// path or use this helper as a public action.
enum class MediaImportMarkerDecision { desktop, skip, duplicate, release };
enum class MediaImportMarkerOutcome {
    marked,       // Durable same-directory rename, original bytes untouched.
    rejected,     // Proven not renamed; never overwrite a collision.
    outcomeUnknown // Rename may have happened: reconcile, do not retry.
};

// Uses the confirmed import source identity, not a pathname from the API.
MediaImportCleanupOutcome cleanupMediaImportSource(
    const LocalMediaImportSource& trustedSource,
    const std::string& sourceEvidence);
// Only call after the Agent has independently confirmed a complete matching
// VDR recording for this exact marked source at execution time.
MediaImportCleanupOutcome cleanupConfirmedDuplicateSource(
    const LocalMediaImportSource& trustedSource,
    const std::string& sourceEvidence);

MediaImportMarkerOutcome markMediaImportSource(
    const LocalMediaImportSource& trustedSource,
    MediaImportMarkerDecision decision);

} // namespace vdrsuite::mediatools
