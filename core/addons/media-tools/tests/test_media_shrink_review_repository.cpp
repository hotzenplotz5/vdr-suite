#include "MediaShrinkReviewRepository.h"
#include "Database.h"
#include "MutationOperation.h"
#include "MutationOperationRepository.h"

#include <cassert>
#include <sqlite3.h>
#include <filesystem>
#include <vector>
#include <unistd.h>
#include <iostream>
#include <string>
using namespace vdrsuite::operations;
using namespace vdrsuite::addons;

namespace {
constexpr const char* recordingId =
    "rec_0123456789abcdef0123456789abcdef";
constexpr const char* reviewId =
    "shk_0123456789abcdef0123456789abcdef";
constexpr const char* newRecordingId =
    "rec_abcdef0123456789abcdef0123456789";

MutationOperation operation(const std::string& id,
                            const std::string& action,
                            const std::string& resourceType,
                            const std::string& resourceId,
                            const std::string& revision,
                            MutationOperationVerificationPolicy verification)
{
    MutationOperation op;
    op.operationId = id;
    op.idempotencyKey = id + "_once";
    op.actorId = "actor-a";
    op.backendId = "backend-a";
    op.backendGeneration = 7;
    op.resourceType = resourceType;
    op.resourceId = resourceId;
    op.expectedRevision = revision;
    op.expectedResourceFingerprint = "source_fingerprint_1";
    op.actionFamily = action;
    op.requestFingerprint = "request_" + id;
    op.requestedAt = 1000;
    op.deadline = 10000;
    op.verificationPolicy = verification;
    op.state = MutationOperationState::accepted;
    op.updatedAt = 1000;
    return op;
}

void succeed(MutationOperationRepository& repo,
             const std::string& id, const std::string& reference)
{
    auto a = repo.transition(id, "1", MutationOperationState::accepted,
        MutationOperationState::queued, "", 1010);
    assert(a.ok());
    a = repo.transition(id, "2", MutationOperationState::queued,
        MutationOperationState::dispatching, "", 1020);
    assert(a.ok());
    a = repo.transition(id, "3", MutationOperationState::dispatching,
        MutationOperationState::executedUnverified, "agent_verified", 1030);
    assert(a.ok());
    a = repo.transition(id, "4", MutationOperationState::executedUnverified,
        MutationOperationState::succeeded, reference, 1040);
    assert(a.ok());
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MutationOperationRepository operations(database);
    assert(operations.ensureSchema());
    MediaShrinkReviewRepository reviews(database, operations);
    assert(reviews.ensureSchema());

    assert(reviews.find("actor-a", "backend-a", reviewId).status ==
           MediaShrinkReviewStatus::notFound);
    assert(reviews.registerStaged(reviewId, "missing").status ==
           MediaShrinkReviewStatus::conflict);
    assert(reviews.registerStaged("../../etc/passwd", "op-stage").status ==
           MediaShrinkReviewStatus::invalid);

    auto stage = operation("op-stage", "media.shrink.stage",
        "Recording", recordingId, "revision_1",
        MutationOperationVerificationPolicy::readbackRequired);
    assert(operations.reserve(stage).ok());
    assert(reviews.registerStaged(reviewId, "op-stage").status ==
           MediaShrinkReviewStatus::conflict);
    succeed(operations, "op-stage", reviewId);
    const auto bound = reviews.registerStaged(reviewId, "op-stage");
    assert(bound.ok() && bound.review.revision == 1 &&
           bound.review.state == "staged");
    assert(reviews.registerStaged(reviewId, "op-stage").status ==
           MediaShrinkReviewStatus::replayed);
    assert(reviews.find("actor-b", "backend-a", reviewId).status ==
           MediaShrinkReviewStatus::notFound);
    assert(reviews.find("actor-a", "backend-b", reviewId).status ==
           MediaShrinkReviewStatus::notFound);

    auto publish = operation("op-publish", "media.shrink.publish",
        "MediaShrinkReview", reviewId, "1",
        MutationOperationVerificationPolicy::reconciliationRequired);
    assert(operations.reserve(publish).ok());
    assert(reviews.claimPublication("actor-b", "backend-a", reviewId,
        1, "op-publish").status == MediaShrinkReviewStatus::notFound);
    const auto claimed = reviews.claimPublication("actor-a","backend-a",
        reviewId, 1, "op-publish");
    assert(claimed.ok() && claimed.review.state == "publishing" &&
           claimed.review.revision == 2);
    assert(reviews.claimPublication("actor-a","backend-a",
        reviewId, 1, "op-publish").status == MediaShrinkReviewStatus::replayed);
    assert(reviews.claimPublication("actor-a","backend-a",
        reviewId, 2, "op-publish").status == MediaShrinkReviewStatus::conflict);
    assert(reviews.completePublication("actor-a","backend-a",
        reviewId, 2).status == MediaShrinkReviewStatus::conflict);
    succeed(operations, "op-publish", newRecordingId);
    assert(reviews.claimPublication("actor-a","backend-a",reviewId,
        1,"op-publish").status == MediaShrinkReviewStatus::conflict);
    const auto completed = reviews.completePublication(
        "actor-a","backend-a",reviewId, 2);
    assert(completed.ok() && completed.review.state == "published" &&
           completed.review.publishedRecordingId == newRecordingId &&
           completed.review.revision == 3);
    assert(reviews.claimPublication("actor-a","backend-a",reviewId,
        3, "op-publish").status == MediaShrinkReviewStatus::conflict);
    assert(reviews.completePublication("actor-b","backend-a",reviewId,
        3).status == MediaShrinkReviewStatus::notFound);
    // Focused P7 Add-on SQLite rehearsal. Persist an unrelated pre-upgrade
    // table and a fully published review through the real backup interface,
    // then rerun the actual schema initializer in the restored database.
    assert(database.execute(
        "CREATE TABLE addon_p7_preexisting(key TEXT PRIMARY KEY,value TEXT);"));
    assert(database.execute(
        "INSERT INTO addon_p7_preexisting VALUES('legacy','unmodified');"));
    Database snapshot;
    assert(snapshot.open(":memory:"));
    sqlite3_backup* backup = sqlite3_backup_init(
        snapshot.handle(), "main", database.handle(), "main");
    assert(backup != nullptr);
    assert(sqlite3_backup_step(backup, -1) == SQLITE_DONE);
    assert(sqlite3_backup_finish(backup) == SQLITE_OK);
    MutationOperationRepository snapshotOperations(snapshot);
    assert(snapshotOperations.ensureSchema());
    MediaShrinkReviewRepository restoredReviews(snapshot, snapshotOperations);
    assert(restoredReviews.ensureSchema());
    assert(restoredReviews.ensureSchema());
    const auto retained = restoredReviews.find("actor-a","backend-a",reviewId);
    assert(retained.ok() && retained.review.state == "published" &&
           retained.review.revision == 3 &&
           retained.review.publishedRecordingId == newRecordingId);
    sqlite3_stmt* legacyRow = nullptr;
    assert(sqlite3_prepare_v2(snapshot.handle(),
        "SELECT COUNT(*) FROM addon_p7_preexisting "
        "WHERE key='legacy' AND value='unmodified';",
        -1, &legacyRow, nullptr) == SQLITE_OK);
    assert(sqlite3_step(legacyRow) == SQLITE_ROW &&
           sqlite3_column_int(legacyRow, 0) == 1);
    assert(sqlite3_finalize(legacyRow) == SQLITE_OK);
    std::cout << "P7_MEDIA_SHRINK_REVIEW_BACKUP=PASS\n";
    // Corrupted persisted rows fail closed; they cannot regain their
    // published/available status merely because the opaque ID still matches.
    assert(database.execute(
        "UPDATE addon_media_shrink_reviews SET state='staged' "
        "WHERE review_id='shk_0123456789abcdef0123456789abcdef';"));
    assert(reviews.find("actor-a","backend-a",reviewId).status ==
           MediaShrinkReviewStatus::unavailable);
    assert(database.execute(
        "UPDATE addon_media_shrink_reviews SET state='published',"
        "published_recording_id='../../untrusted' "
        "WHERE review_id='shk_0123456789abcdef0123456789abcdef';"));
    assert(reviews.find("actor-a","backend-a",reviewId).status ==
           MediaShrinkReviewStatus::unavailable);
    // Restart recovery: review and its publication claim survive process
    // lifetime. Only canonical Core operations can promote either phase.
    std::string pattern = (std::filesystem::temp_directory_path() /
        "vdr-suite-shrink-review-XXXXXX").string();
    std::vector<char> filename(pattern.begin(), pattern.end());
    filename.push_back('\0');
    const int fd = ::mkstemp(filename.data());
    assert(fd >= 0 && ::close(fd) == 0);
    struct Cleanup {
        std::string name;
        ~Cleanup() { std::error_code ec; std::filesystem::remove(name, ec); }
    } cleanup{filename.data()};
    {
        Database first;
        assert(first.open(cleanup.name));
        MutationOperationRepository operations1(first);
        assert(operations1.ensureSchema());
        MediaShrinkReviewRepository reviews1(first, operations1);
        assert(reviews1.ensureSchema());
        auto stage = operation("op-restart-stage", "media.shrink.stage",
            "Recording", recordingId, "revision_1",
            MutationOperationVerificationPolicy::readbackRequired);
        assert(operations1.reserve(stage).ok());
        succeed(operations1,"op-restart-stage",reviewId);
        assert(reviews1.registerStaged(reviewId,"op-restart-stage").ok());
    }
    {
        Database second;
        assert(second.open(cleanup.name));
        MutationOperationRepository operations2(second);
        assert(operations2.ensureSchema());
        MediaShrinkReviewRepository reviews2(second, operations2);
        assert(reviews2.ensureSchema());
        const auto persisted = reviews2.find("actor-a","backend-a",reviewId);
        assert(persisted.ok() && persisted.review.state == "staged" &&
               persisted.review.revision == 1);
        auto publish = operation("op-restart-publish","media.shrink.publish",
            "MediaShrinkReview",reviewId,"1",
            MutationOperationVerificationPolicy::reconciliationRequired);
        assert(operations2.reserve(publish).ok());
        assert(reviews2.claimPublication("actor-a","backend-a",reviewId,
            1,"op-restart-publish").ok());
    }
    {
        Database third;
        assert(third.open(cleanup.name));
        MutationOperationRepository operations3(third);
        assert(operations3.ensureSchema());
        MediaShrinkReviewRepository reviews3(third, operations3);
        assert(reviews3.ensureSchema());
        assert(reviews3.claimPublication("actor-a","backend-a",reviewId,
            1,"op-restart-publish").status == MediaShrinkReviewStatus::replayed);
        assert(reviews3.claimPublication("actor-a","backend-a",reviewId,
            2,"op-restart-publish").status == MediaShrinkReviewStatus::conflict);
        assert(reviews3.claimPublication("actor-a","backend-a",reviewId,
            1,"op-other-publish").status == MediaShrinkReviewStatus::conflict);
        assert(reviews3.completePublication("actor-a","backend-a",reviewId,
            2).status == MediaShrinkReviewStatus::conflict);
        succeed(operations3,"op-restart-publish",newRecordingId);
        assert(reviews3.claimPublication("actor-a","backend-a",reviewId,
            1,"op-restart-publish").status == MediaShrinkReviewStatus::conflict);
        const auto completed = reviews3.completePublication(
            "actor-a","backend-a",reviewId,2);
        assert(completed.ok() && completed.review.state == "published" &&
               completed.review.publishedRecordingId == newRecordingId);
    }
    std::cout << "MEDIA_SHRINK_CORE_REVIEW_TEST=PASS\n";
}
