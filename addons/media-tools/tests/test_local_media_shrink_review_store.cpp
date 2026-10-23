#include "LocalMediaShrinkReviewStore.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace vdrsuite::mediatools;

int main() {
    std::string pattern =
        (fs::temp_directory_path() / "suite-shrink-opaque-test-XXXXXX").string();
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    const char* temporary = ::mkdtemp(buffer.data());
    assert(temporary != nullptr);
    const fs::path home(temporary);
    struct Cleanup {
        fs::path directory;
        ~Cleanup() {
            std::error_code ignored;
            fs::remove_all(directory, ignored);
        }
    } cleanup{home};

    const auto state = home / "agent-state";
    const auto root = home / "video";
    const auto staged = root / ".vdr-suite-shrink-aabbcc";
    const auto candidate = staged / "2026-10-23.08.00.1-0.rec";
    fs::create_directories(state);
    fs::create_directories(candidate);
    assert(::chmod(state.c_str(), 0700) == 0);
    assert(::chmod(staged.c_str(), 0700) == 0);
    assert(::chmod(candidate.c_str(), 0700) == 0);
    std::ofstream(candidate / "00001.ts") << "HEVC TEST ONLY";

    LocalMediaShrinkReviewStore store(state);
    LocalMediaShrinkReview original;
    original.actorId = "actor-a";
    original.backendId = "backend-a";
    original.recordingId = "rec_0123456789abcdef0123456789abcdef";
    original.stageOperationId = "op_stage-a";
    original.sourceFingerprint = "source-fingerprint-123";
    original.backendGeneration = 7;
    original.videoRoot = root;
    original.relativeRecording = fs::path("Example") / candidate.filename();
    original.candidateDirectory = candidate;
    // Persisted Stage attempt identity is irreversible, including a
    // simulated Agent restart and an altered payload with the same operation.
    assert(!store.claimStageOnce(
        "../../unsafe", original.actorId, original.backendId,
        original.recordingId, original.sourceFingerprint, 7));
    assert(store.claimStageOnce(
        original.stageOperationId, original.actorId, original.backendId,
        original.recordingId, original.sourceFingerprint, 7));
    assert(!store.claimStageOnce(
        original.stageOperationId, original.actorId, original.backendId,
        original.recordingId, "different-source", 7));
    LocalMediaShrinkReviewStore restartedBeforePublish(state);
    assert(!restartedBeforePublish.claimStageOnce(
        original.stageOperationId, original.actorId, original.backendId,
        original.recordingId, original.sourceFingerprint, 7));
    assert(restartedBeforePublish.claimStageOnce(
        "op_stage-b", original.actorId, original.backendId,
        original.recordingId, original.sourceFingerprint, 7));

    const auto id = store.registerCandidate(original);
    assert(id.size() == 36 && id.rfind("shk_", 0) == 0);
    assert(!store.find("../../etc/passwd"));
    const auto saved = store.find(id);
    assert(saved && saved->reviewId == id &&
           saved->actorId == "actor-a" &&
           saved->backendGeneration == 7 &&
           saved->candidateDirectory == candidate &&
           saved->sourceFingerprint == original.sourceFingerprint);
    // Durable state survives a fresh store instance.
    LocalMediaShrinkReviewStore reopened(state);
    assert(reopened.find(id));
    assert(reopened.claimOnce(id, "op_publish-a"));
    // Publication cannot be replayed by a restarted Agent, even if the
    // execution status is unknown after a process crash.
    assert(!store.claimOnce(id, "op_publish-a"));
    assert(!store.claimOnce(id, "op_publish-b"));
    assert(!store.claimOnce(id, "../../evil"));

    // The internal record can never be read via a symlink to an attacker
    // controlled file, and permission drift fails closed.
    fs::create_symlink(id, state / "shk_ffffffffffffffffffffffffffffffff");
    assert(!store.find("shk_ffffffffffffffffffffffffffffffff"));
    assert(::chmod(state.c_str(), 0777) == 0);
    assert(!store.find(id));
    assert(!store.claimOnce(id, "op_publish-c"));
    assert(::chmod(state.c_str(), 0700) == 0);
    assert(store.find(id));

    // Candidate replacement or removal invalidates local evidence.
    fs::rename(candidate, staged / "saved.rec");
    assert(!store.find(id));
    fs::rename(staged / "saved.rec", candidate);
    assert(store.find(id));

    std::cout << "LOCAL_HEVC_OPAQUE_REVIEW=PASS "
              << "DURABLE_STAGE_FENCE=PASS DURABLE_PUBLISH_FENCE=PASS ORIGINAL_PRESERVED=YES\n";
}
