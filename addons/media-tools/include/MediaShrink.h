#pragma once

#include "MediaImport.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace vdrsuite::mediatools {

// Separate HEVC operation on existing VDR recordings. No original recording
// is removed or overwritten, including at the explicit publication boundary.
// Only trusted backend-local callers may supply a video root or recording path.
enum class ShrinkEncoder { software, vaapi, qsv, nvenc };

struct ShrinkOptions {
    ShrinkEncoder encoder = ShrinkEncoder::software;
    unsigned maxHeight = 0; // 0: preserve size; else 480..2160
    bool nightMode = false;
};

struct ShrinkSegment {
    std::string filename;
    std::uint64_t device = 0, inode = 0, bytes = 0;
    std::int64_t seconds = 0, nanoseconds = 0;
};

struct ShrinkPlan {
    fs::path videoRoot;
    fs::path relativeRecording;
    std::vector<ShrinkSegment> segments;
    // Immutable evidence for native index, info and existing SRT sidecars.
    // Publication must reject sidecar edits made after review staging.
    std::vector<ShrinkSegment> sidecars;
    std::uint64_t totalBytes = 0;
    ShrinkOptions options;
};

// Pure read-only validation and total source-size measurement. Only numbered
// contiguous .ts segments from the trusted .rec directory are accepted.
ShrinkPlan inspectShrink(
    const fs::path& videoRoot, const fs::path& relativeRecording,
    const ShrinkOptions& options = {});

// Stable, path-free identity of the inspected source snapshot and sidecars.
// Recomputed at each Core/Agent mutation boundary, never supplied by HTTP.
std::string shrinkSourceFingerprint(const ShrinkPlan& plan);

void requireShrinkCapacity(const ShrinkPlan& plan);

// The first explicit write action only creates a private review candidate.
// No VDR recording path is changed, and no root/source file is removed.
fs::path stageShrink(
    const ShrinkPlan& plan, ProcessRunner& runner,
    const std::function<void(const ShrinkPlan&)>& capacityPreflight);

// Durable, opaque Agent-to-Core review binding kept INSIDE the private
// candidate until publication. This is NOT a mutation credential and is
// never exposed in browser payloads or placed in the public VDR recording.
struct ShrinkReviewBinding {
    std::string reviewId, stageOperationId, actorId, backendId, recordingId;
    std::string sourceFingerprint;
    std::uint64_t backendGeneration = 0;
};
void writeShrinkReviewBinding(
    const fs::path& candidateDirectory, const ShrinkReviewBinding& binding);
std::optional<ShrinkReviewBinding> readShrinkReviewBinding(
    const fs::path& candidateDirectory);

// Independent read-only verification on a fresh source snapshot. Can be
// used by Core reconciliation without publishing or changing original files.
// Throws on missing, modified, foreign or incomplete HEVC candidates.
std::uint64_t verifyShrinkCandidate(
    const ShrinkPlan& source, const fs::path& candidateDirectory,
    ProcessRunner& runner);

// A SECOND explicit action publishes a fully verified candidate as a new
// .rec with RENAME_NOREPLACE. The old recording is retained indefinitely;
// any later original deletion belongs to Suite's existing Recording owner.
fs::path publishShrinkCandidate(
    const ShrinkPlan& source, const fs::path& candidateDirectory,
    ProcessRunner& runner);

} // namespace vdrsuite::mediatools
