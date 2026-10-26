#include "MediaShrinkOperationAdmission.h"

#include "Database.h"
#include "MediaShrinkReviewRepository.h"
#include "MutationOperation.h"
#include "MutationOperationRepository.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

using namespace vdrsuite::addons;
using namespace vdrsuite::operations;
namespace {
constexpr const char* recording = "rec_0123456789abcdef0123456789abcdef";
constexpr const char* reviewId = "shk_0123456789abcdef0123456789abcdef";

struct Authority : IMediaShrinkAdmissionAuthority {
    bool enabledStage = false;
    bool enabledPublish = false;
    std::uint64_t generation = 7;
    bool allowStage(const std::string& actor, const std::string& backend,
                    const std::string& rec, const std::string& source,
                    std::int64_t now, std::uint64_t& out) override {
        if (!enabledStage || actor != "actor-a" || backend != "backend-a" ||
            rec != recording || source != "source_fingerprint_1" || now <= 0)
            return false;
        out = generation;
        return true;
    }
    bool allowPublish(const std::string& actor, const std::string& backend,
                      const std::string& review, const std::string& source,
                      std::int64_t now, std::uint64_t& out) override {
        if (!enabledPublish || actor != "actor-a" || backend != "backend-a" ||
            review != reviewId || source != "source_fingerprint_1" || now <= 0)
            return false;
        out = generation;
        return true;
    }
};

void succeed(MutationOperationRepository& operations,
             const std::string& id, const std::string& reference) {
    auto x = operations.transition(id, "1", MutationOperationState::accepted,
        MutationOperationState::queued, "", 1010);
    assert(x.ok());
    x = operations.transition(id, "2", MutationOperationState::queued,
        MutationOperationState::dispatching, "", 1020);
    assert(x.ok());
    x = operations.transition(id, "3", MutationOperationState::dispatching,
        MutationOperationState::executedUnverified, "agent_receipt", 1030);
    assert(x.ok());
    x = operations.transition(id, "4", MutationOperationState::executedUnverified,
        MutationOperationState::succeeded, reference, 1040);
    assert(x.ok());
}
}

int main() {
    Database database;
    assert(database.open(":memory:"));
    MutationOperationRepository operations(database);
    assert(operations.ensureSchema());
    MediaShrinkReviewRepository reviews(database, operations);
    assert(reviews.ensureSchema());
    Authority authority;
    MediaShrinkOperationAdmissionService service(operations, reviews, authority);

    MediaShrinkStageAdmission stage;
    stage.actorId = "actor-a";
    stage.backendId = "backend-a";
    stage.recordingId = recording;
    stage.sourceFingerprint = "source_fingerprint_1";
    stage.idempotencyKey = "stage_request_001";
    stage.encoder = "software";
    stage.maxHeight = 720;
    stage.now = 1000;

    assert(service.stage(stage).status == MediaShrinkAdmissionStatus::forbidden);
    assert(operations.listByActionFamilyAndStates("media.shrink.stage",
        {MutationOperationState::accepted}, 10).operations.empty());
    authority.enabledStage = true;
    auto first = service.stage(stage);
    assert(first.status == MediaShrinkAdmissionStatus::accepted);
    assert(first.operation.actionFamily == "media.shrink.stage");
    assert(first.operation.resourceType == "Recording");
    assert(first.operation.expectedRevision == "source-v1");
    assert(first.operation.verificationPolicy ==
           MutationOperationVerificationPolicy::readbackRequired);
    assert(first.operation.deadline >= 1000 + 3600);
    const auto p = operations.findPayloadByOperationId(first.operation.operationId);
    assert(p.ok());
    assert(p.payload.payloadType == "addon.media.shrink.stage");
    assert(p.payload.payload.find("rec_0123456789abcdef") != std::string::npos);
    assert(p.payload.payload.find('/') == std::string::npos);
    assert(p.payload.payloadFingerprint != first.operation.requestFingerprint);

    auto replay = service.stage(stage);
    assert(replay.status == MediaShrinkAdmissionStatus::replayed);
    assert(replay.operation.operationId == first.operation.operationId);
    auto modified = stage;
    modified.encoder = "vaapi";
    assert(service.stage(modified).status ==
           MediaShrinkAdmissionStatus::idempotencyConflict);
    modified = stage;
    modified.recordingId = "../another.rec";
    assert(service.stage(modified).status == MediaShrinkAdmissionStatus::invalid);
    modified = stage;
    modified.idempotencyKey = "short";
    assert(service.stage(modified).status == MediaShrinkAdmissionStatus::invalid);
    authority.generation = 8;
    assert(service.stage(stage).status == MediaShrinkAdmissionStatus::idempotencyConflict);
    authority.generation = 7;

    // Stage operation cannot become a review without independent canonical
    // completion; the test's synthetic verifier transitions it explicitly.
    assert(reviews.registerStaged(reviewId, first.operation.operationId).status ==
           MediaShrinkReviewStatus::conflict);
    succeed(operations, first.operation.operationId, reviewId);
    assert(reviews.registerStaged(reviewId, first.operation.operationId).ok());

    MediaShrinkPublishAdmission publication;
    publication.actorId = "actor-a";
    publication.backendId = "backend-a";
    publication.reviewId = reviewId;
    publication.expectedRevision = 1;
    publication.idempotencyKey = "publish_request_001";
    publication.now = 1100;
    assert(service.publish(publication).status ==
           MediaShrinkAdmissionStatus::forbidden);
    authority.enabledPublish = true;
    auto accepted = service.publish(publication);
    assert(accepted.status == MediaShrinkAdmissionStatus::accepted);
    assert(accepted.operation.actionFamily == "media.shrink.publish");
    assert(accepted.operation.verificationPolicy ==
           MutationOperationVerificationPolicy::reconciliationRequired);
    assert(accepted.operation.expectedRevision == "1");
    assert(reviews.find("actor-a", "backend-a", reviewId).review.state ==
           "publishing");
    replay = service.publish(publication);
    assert(replay.status == MediaShrinkAdmissionStatus::replayed);
    assert(replay.operation.operationId == accepted.operation.operationId);

    auto wrong = publication;
    wrong.idempotencyKey = "publish_second_001";
    assert(service.publish(wrong).status ==
           MediaShrinkAdmissionStatus::reviewConflict);
    wrong = publication;
    wrong.actorId = "actor-b";
    assert(service.publish(wrong).status ==
           MediaShrinkAdmissionStatus::reviewConflict);
    wrong = publication;
    wrong.expectedRevision = 2;
    assert(service.publish(wrong).status == MediaShrinkAdmissionStatus::invalid);
    authority.generation = 8;
    assert(service.publish(publication).status ==
           MediaShrinkAdmissionStatus::forbidden);

    std::cout << "MEDIA_SHRINK_CORE_ADMISSION=PASS\n";
}
