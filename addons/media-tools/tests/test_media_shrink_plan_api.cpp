#include "DaemonMediaShrinkPlanRuntime.h"
#include "MediaShrinkPlanApiRuntime.h"
#include "Database.h"
#include "MutationOperationRepository.h"
#include "MediaShrinkReviewRepository.h"

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
    DaemonMediaShrinkPlanRuntime runtime(
        {{"local", root.string()}},
        [&](const std::string& backend, const std::string& id)
            -> std::optional<fs::path> {
            if (backend == "local" && id == kRecordingId)
                return recording;
            return std::nullopt;
        },
        [](const std::string& backend) {return backend == "local";},
        &reviews);
    auto& api = MediaShrinkPlanApiRuntime::instance();
    api.registerHandlers(
        [&](const std::string& actor, const std::string& backend,
            const std::string& id) {
            return runtime.plan(actor, backend, id);
        },
        [&](const std::string& backend) {
            return runtime.capability(backend);
        },
        [&](const std::string& actor, const std::string& backend,
            const std::string& id) {
            return runtime.reviewStatus(actor, backend, id);
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

    std::set<std::string> names;
    for (const auto& item : fs::directory_iterator(recording))
        names.insert(item.path().filename().string());
    assert(names == originalNames);
    std::ifstream in(recording / "00001.ts", std::ios::binary);
    const std::string after{std::istreambuf_iterator<char>(in),
                            std::istreambuf_iterator<char>()};
    assert(before == after);
    // Failed-open if a recording path is replaced by a symlink.
    fs::rename(recording / "00001.ts", recording / "saved.ts");
    fs::create_symlink(recording / "saved.ts", recording / "00001.ts");
    assert(post(request, "actor-a", "local").statusCode == 422);
    api.registerHandlers({}, {});
}
