#include "DaemonMediaShrinkPlanRuntime.h"
#include "MediaShrinkPlanApiRuntime.h"
#include "Database.h"
#include "MutationOperationRepository.h"
#include "MediaShrinkReviewRepository.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
constexpr const char* kRecordingId = "rec_0123456789abcdef0123456789abcdef";
constexpr const char* kReviewId = "shk_0123456789abcdef0123456789abcdef";

// Stub native process output is confined to this test's disposable recording.
// Core readback must NOT invoke FFmpeg or the VDR index generator again.
struct FakeShrinkRunner final : vdrsuite::mediatools::ProcessRunner {
    unsigned encodes = 0;
    unsigned indexes = 0;

    vdrsuite::mediatools::ProcessResult run(
        const std::vector<std::string>& args, bool captured) override {
        const auto has = [&](const std::string& value) {
            return std::find(args.begin(), args.end(), value) != args.end();
        };
        if (args.at(0) == "/usr/bin/ffprobe") {
            assert(captured);
            if (has("stream=codec_name:format=format_name"))
                return {0, "codec_name=aac\nformat_name=mpegts\n"};
            if (has("stream=codec_name")) {
                if (has("a")) return {0, "aac\n"};
                return {0, args.back().find("input.ts") != std::string::npos
                    ? "h264\n" : "hevc\n"};
            }
            if (has("stream=profile,pix_fmt"))
                return {0, "profile=Main\npix_fmt=yuv420p\n"};
            if (has("format=duration")) return {0, "60.0\n"};
            if (has("stream=height")) return {0, "576\n"};
        }
        if (args.at(0) == "/usr/bin/ffmpeg") {
            assert(!captured);
            ++encodes;
            std::ofstream(args.back(), std::ios::binary)
                << std::string(8192, 'H');
            return {0, ""};
        }
        if (args.at(0) == "/usr/bin/vdr") {
            assert(!captured);
            const std::string prefix = "--genindex=";
            assert(args.size() == 2 &&
                   args[1].compare(0, prefix.size(), prefix) == 0);
            ++indexes;
            std::ofstream(fs::path(args[1].substr(prefix.size())) / "index")
                << "INDEX";
            return {0, ""};
        }
        return {1, ""};
    }
};

int main() {
    const auto sample = (fs::temp_directory_path() /
        "vdr-suite-shrink-plan-XXXXXX").string();
    std::vector<char> buffer(sample.begin(), sample.end());
    buffer.push_back('\0');
    const char* created = ::mkdtemp(buffer.data());
    assert(created);
    const fs::path home(created);
    struct Cleanup {
        fs::path home;
        ~Cleanup() { std::error_code e; fs::remove_all(home, e); }
    } cleanup{home};
    const fs::path root = home / "video";
    const fs::path recording = root / "Example" /
        "2026-10-22.12.00.1-0.rec";
    fs::create_directories(recording);
    std::ofstream(recording / "00001.ts", std::ios::binary)
        << std::string(32768, 'X');
    std::ofstream(recording / "info") << "T Example\n";
    std::ofstream(recording / "index") << "native-index";
    const std::set<std::string> originalNames = {
        "00001.ts", "info", "index"
    };
    std::string before;
    {
        std::ifstream in(recording / "00001.ts", std::ios::binary);
        before.assign(std::istreambuf_iterator<char>(in),
                      std::istreambuf_iterator<char>());
    }
    Database database;
    assert(database.open((home / "suite.sqlite").string()));
    vdrsuite::operations::MutationOperationRepository operations(database);
    assert(operations.ensureSchema());
    vdrsuite::addons::MediaShrinkReviewRepository reviews(database, operations);
    assert(reviews.ensureSchema());
    using namespace vdrsuite::operations;
    MutationOperation stage;
    stage.operationId = "op_shrink_stage";
    stage.idempotencyKey = "shrink_stage_once";
    stage.actorId = "actor-a";
    stage.backendId = "local";
    stage.backendGeneration = 1;
    stage.resourceType = "Recording";
    stage.resourceId = kRecordingId;
    stage.actionFamily = "media.shrink.stage";
    stage.expectedRevision = "rec-rev-1";
    stage.expectedResourceFingerprint = "source_fingerprint_1";
    stage.requestFingerprint = "request_stage_1";
    stage.requestedAt = 100;
    stage.deadline = 1000;
    stage.updatedAt = 100;
    stage.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;
    assert(operations.reserve(stage).ok());
    assert(operations.transition(stage.operationId, "1",
        MutationOperationState::accepted, MutationOperationState::queued,
        "", 110).ok());
    assert(operations.transition(stage.operationId, "2",
        MutationOperationState::queued, MutationOperationState::dispatching,
        "", 120).ok());
    assert(operations.transition(stage.operationId, "3",
        MutationOperationState::dispatching,
        MutationOperationState::executedUnverified,
        "agent_evidence", 130).ok());
    assert(operations.transition(stage.operationId, "4",
        MutationOperationState::executedUnverified,
        MutationOperationState::succeeded, kReviewId, 140).ok());
    assert(reviews.registerStaged(kReviewId, stage.operationId).ok());
    bool backendOnline = true;
    DaemonMediaShrinkPlanRuntime runtime(
        {{"local", root.string()}},
        [&](const std::string& backend, const std::string& id)
            -> std::optional<fs::path> {
            if (backend == "local" && id == kRecordingId)
                return recording;
            return std::nullopt;
        },
        [&](const std::string& backend) {
            return backendOnline && backend == "local";
        },
        &reviews);
    auto& api = MediaShrinkPlanApiRuntime::instance();
    api.registerHandlers(
        [&](const std::string& actor, const std::string& backend,
            const std::string& id) {
            return runtime.plan(actor, backend, id);
        },
        [&](const std::string& actor, const std::string& backend) {
            assert(!actor.empty());
            return runtime.capability(backend);
        },
        [&](const std::string& actor, const std::string& backend,
            const std::string& id) {
            return runtime.reviewStatus(actor, backend, id);
        });
    // Recovery lookup uses the same durable operation repository and
    // actor/backend scope as the existing operation read, not browser storage.
    api.registerRecordingStatusHandler(
        [&](const std::string& actor, const std::string& backend,
            const std::string& id) -> ApiResponse {
            const auto found =
                operations.listRecentForActorBackendActionFamily(
                    actor, backend, "media.shrink.stage", 128U);
            if (!found.ok()) return MediaShrinkPlanApiRuntime::problem(
                503, "Unavailable.");
            for (const auto& op : found.operations) {
                if (op.resourceId != id || op.resourceType != "Recording" ||
                    op.state != MutationOperationState::succeeded)
                    continue;
                const auto review = reviews.find(actor, backend, op.resultReference);
                if (!review.ok()) continue;
                return MediaShrinkPlanApiRuntime::json(200,
                    "{\"recordingId\":\"" + id +
                    "\",\"state\":\"ready\",\"reviewId\":\"" +
                    review.review.reviewId + "\"}");
            }
            return MediaShrinkPlanApiRuntime::json(200,
                "{\"recordingId\":\"" + id +
                "\",\"state\":\"none\"}");
        });
    auto post = [&](const std::string& body, const std::string& actor,
                    const std::string& scope) {
        ApiResponse response;
        assert(api.tryHandlePost(
            "/api/v1/backends/local/media-shrink-plans",
            body, actor, scope, "application/json", response));
        return response;
    };
    const std::string request = std::string("{\"recordingId\":\"") +
        kRecordingId + "\"}";
    assert(post(request, "", "local").statusCode == 401);
    assert(post(request, "actor-a", "elsewhere").statusCode == 403);
    assert(post("{\"recordingId\":\"../../etc/passwd\"}",
                "actor-a", "local").statusCode == 422);
    assert(post(request.substr(0, request.size() - 1) +
                ",\"path\":\"/tmp/evil\"}", "actor-a", "local").statusCode == 422);
    assert(post("{\"recordingId\":\"rec_ffffffffffffffffffffffffffffffff\"}",
                "actor-a", "local").statusCode == 404);
    ApiResponse capability;
    assert(api.tryHandleGet(
        "/api/v1/backends/local/capabilities/media-shrink",
        "actor-a", "local", {}, capability));
    assert(capability.statusCode == 200);
    assert(capability.body.find("\"planningAvailable\":true") !=
           std::string::npos);
    assert(capability.body.find("\"available\":false") !=
           std::string::npos);
    assert(capability.body.find(
        "\"encoders\":[\"software\",\"vaapi\",\"qsv\",\"nvenc\"]") !=
        std::string::npos);
    assert(capability.body.find("\"encoderFallbackToSoftware\":true") !=
           std::string::npos);
    assert(api.tryHandleGet(
        "/api/v1/backends/local/capabilities/media-shrink",
        "actor-a", "other", {}, capability));
    assert(capability.statusCode == 404);
    const auto reply = post(request, "actor-a", "local");
    assert(reply.statusCode == 200);
    assert(reply.body.find("\"readOnly\":true") != std::string::npos);
    assert(reply.body.find("\"stageAvailable\":false") != std::string::npos);
    assert(reply.body.find("\"sourceBytes\":32768") != std::string::npos);
    assert(reply.body.find("\"segments\":1") != std::string::npos);
    assert(reply.body.find(home.string()) == std::string::npos);
    assert(reply.body.find("2026-10-22.12.00.1-0.rec") == std::string::npos);

    auto readReview = [&](const std::string& actor,
                          const std::string& scope, const std::string& id) {
        ApiResponse response;
        assert(api.tryHandleGet(
            "/api/v1/backends/local/media-shrink-reviews/" + id,
            actor, scope, {}, response));
        return response;
    };
    assert(readReview("", "local", kReviewId).statusCode == 401);
    assert(readReview("actor-b", "local", kReviewId).statusCode == 404);
    assert(readReview("actor-a", "elsewhere", kReviewId).statusCode == 404);
    assert(readReview("actor-a", "local", "../../etc/passwd").statusCode == 404);
    auto readRecording = [&](const std::string& actor,
                             const std::string& scope,
                             const std::string& id) {
        ApiResponse response;
        assert(api.tryHandleGet(
            "/api/v1/backends/local/media-shrink-recordings/" + id,
            actor, scope, {}, response));
        return response;
    };
    assert(readRecording("", "local", kRecordingId).statusCode == 401);
    assert(readRecording("actor-a", "elsewhere", kRecordingId).statusCode == 404);
    assert(readRecording("actor-a", "local", "../../etc/passwd").statusCode == 404);
    const auto existing = readRecording("actor-a", "local", kRecordingId);
    assert(existing.statusCode == 200);
    assert(existing.body.find("\"state\":\"ready\"") !=
           std::string::npos);
    assert(existing.body.find(kReviewId) != std::string::npos);
    const auto foreign = readRecording("actor-b", "local", kRecordingId);
    assert(foreign.statusCode == 200);
    assert(foreign.body.find("\"state\":\"none\"") !=
           std::string::npos);

    const auto staged = readReview("actor-a", "local", kReviewId);
    assert(staged.statusCode == 200);
    assert(staged.body.find("\"state\":\"staged\"") != std::string::npos);
    assert(staged.body.find("\"revision\":1") != std::string::npos);
    assert(staged.body.find("\"publishAvailable\":false") != std::string::npos);
    assert(staged.body.find(home.string()) == std::string::npos);
    assert(staged.body.find("source_fingerprint") == std::string::npos);
    MutationOperation publish = stage;
    publish.operationId = "op_shrink_publish";
    publish.idempotencyKey = "shrink_publish_once";
    publish.resourceType = "MediaShrinkReview";
    publish.resourceId = kReviewId;
    publish.expectedRevision = "1";
    publish.actionFamily = "media.shrink.publish";
    publish.requestFingerprint = "request_publish_1";
    publish.verificationPolicy =
        MutationOperationVerificationPolicy::reconciliationRequired;
    assert(operations.reserve(publish).ok());
    assert(reviews.claimPublication(
        "actor-a", "local", kReviewId, 1, publish.operationId).ok());
    const auto publishing = readReview("actor-a", "local", kReviewId);
    assert(publishing.statusCode == 200);
    assert(publishing.body.find("\"state\":\"publishing\"") != std::string::npos);
    assert(publishing.body.find("\"revision\":2") != std::string::npos);

    // A completed x265 stage can outlive its Agent generation and leave
    // only a durable outcome_unknown result. Recover its opaque review ID
    // from the private Core-bound marker, never from the browser or a new
    // execution, and verify the source/candidate independently.
    {
        using namespace vdrsuite::mediatools;
        const auto relative = recording.lexically_relative(root);
        const auto source = inspectShrink(root, relative);
        const auto fingerprint = shrinkSourceFingerprint(source);
        const std::string recoveredOperation =
            "op_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        const std::string recoveredReview =
            "shk_bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
        FakeShrinkRunner runner;
        const auto candidate = stageShrink(source, runner);
        writeShrinkReviewBinding(candidate, ShrinkReviewBinding{
            recoveredReview, recoveredOperation, "actor-a",
            "local", kRecordingId, fingerprint, 7});
        // The installed Core daemon runs as root while the Agent creates
        // private candidates as vdr. Reproduce this real UID separation
        // when the fixture itself is being run by root.
        if (::geteuid() == 0) {
            constexpr uid_t agentOwner = 65534;
            assert(::chown(recording.c_str(), agentOwner, static_cast<gid_t>(-1)) == 0);
            assert(::chown(candidate.parent_path().c_str(),
                           agentOwner, static_cast<gid_t>(-1)) == 0);
            assert(::chown(candidate.c_str(), agentOwner,
                           static_cast<gid_t>(-1)) == 0);
            assert(::chown((candidate / ".shrink-core-review").c_str(),
                           agentOwner, static_cast<gid_t>(-1)) == 0);
        }
        assert(runner.encodes == 1 && runner.indexes == 1);
        const auto recovered = runtime.recoverStagedCandidate(
            "actor-a", "local", kRecordingId, recoveredOperation,
            fingerprint, 7, runner);
        assert(recovered && recovered->reviewId == recoveredReview);
        assert(recovered->sourceBytes == 32768 &&
               recovered->candidateBytes == 8192);
        // A Stage completed under generation 7 before the Agent dropped
        // offline. Core must still verify its private immutable candidate.
        // New Stage/Publish admissions remain gated by live capabilities.
        backendOnline = false;
        assert(!runtime.trustedSourcePaths("local", kRecordingId, fingerprint));
        assert(!runtime.trustedSourceFingerprint("local", kRecordingId));
        std::string readbackDiagnostic;
        const auto recoveredOffline = runtime.recoverStagedCandidate(
            "actor-a", "local", kRecordingId, recoveredOperation,
            fingerprint, 7, runner, &readbackDiagnostic);
        assert(recoveredOffline &&
               recoveredOffline->reviewId == recoveredReview);
        assert(readbackDiagnostic.empty());
        const auto wrongOperationWhileOffline =
            runtime.recoverStagedCandidate("actor-a", "local", kRecordingId,
                "op_eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                fingerprint, 7, runner, &readbackDiagnostic);
        assert(!wrongOperationWhileOffline);
        assert(readbackDiagnostic == "private_review_not_found_for_operation");
        assert(runner.encodes == 1 && runner.indexes == 1);
        backendOnline = true;
        const auto exact = runtime.verifyStagedCandidate(
            "actor-a", "local", kRecordingId, recoveredOperation,
            fingerprint, 7, recoveredReview, runner);
        assert(exact && exact->reviewId == recoveredReview);
        assert(!runtime.recoverStagedCandidate(
            "actor-b", "local", kRecordingId, recoveredOperation,
            fingerprint, 7, runner));
        assert(!runtime.recoverStagedCandidate(
            "actor-a", "local", kRecordingId,
            "op_cccccccccccccccccccccccccccccccc",
            fingerprint, 7, runner));
        assert(!runtime.recoverStagedCandidate(
            "actor-a", "local", kRecordingId, recoveredOperation,
            fingerprint, 8, runner));
        assert(!runtime.verifyStagedCandidate(
            "actor-a", "local", kRecordingId, recoveredOperation,
            fingerprint, 7, kReviewId, runner));
        assert(runner.encodes == 1 && runner.indexes == 1);

        // Two Core-bound reviews for the SAME operation are ambiguous.
        // Never pick the first directory arbitrarily.
        const auto competing = stageShrink(source, runner);
        writeShrinkReviewBinding(competing, ShrinkReviewBinding{
            "shk_cccccccccccccccccccccccccccccccc",
            recoveredOperation, "actor-a", "local",
            kRecordingId, fingerprint, 7});
        assert(!runtime.recoverStagedCandidate(
            "actor-a", "local", kRecordingId, recoveredOperation,
            fingerprint, 7, runner));
        assert(runner.encodes == 2 && runner.indexes == 2);
        // Readback did not touch the original recording or re-encode.
    }

    std::set<std::string> names;
    for (const auto& item : fs::directory_iterator(recording))
        names.insert(item.path().filename().string());
    assert(names == originalNames);
    std::ifstream in(recording / "00001.ts", std::ios::binary);
    const std::string after{std::istreambuf_iterator<char>(in),
                            std::istreambuf_iterator<char>()};
    assert(before == after);
    // After a completed Stage, metadata enrichment may rewrite the original
    // info file before Core reads back a previously published VDR recording.
    // The read-only Publish path still resolves the trusted native identity,
    // but must not require the obsolete Stage source fingerprint.
    const auto relative = recording.lexically_relative(root);
    const auto stageFingerprint = vdrsuite::mediatools::shrinkSourceFingerprint(
        vdrsuite::mediatools::inspectShrink(root, relative));
    assert(runtime.trustedSourcePaths("local", kRecordingId, stageFingerprint));
    std::ofstream(recording / "info", std::ios::app)
        << "D Metadata nach Stage aktualisiert\n";
    assert(!runtime.trustedSourcePaths("local", kRecordingId, stageFingerprint));
    const auto publishedReadback = runtime.trustedSourcePaths(
        "local", kRecordingId, stageFingerprint, true, false);
    assert(publishedReadback && publishedReadback->videoRoot == root &&
           publishedReadback->relativeRecording == relative);
    assert(!runtime.trustedSourcePaths("local", "rec_wrong",
                                       stageFingerprint, true, false));
    // Failed-open if a recording path is replaced by a symlink.
    fs::rename(recording / "00001.ts", recording / "saved.ts");
    fs::create_symlink(recording / "saved.ts", recording / "00001.ts");
    assert(post(request, "actor-a", "local").statusCode == 422);
    api.registerHandlers({}, {});
}
