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

MediaImportMarkerOutcome markMediaImportSource(
    const LocalMediaImportSource& trustedSource,
    MediaImportMarkerDecision decision);

} // namespace vdrsuite::mediatools
