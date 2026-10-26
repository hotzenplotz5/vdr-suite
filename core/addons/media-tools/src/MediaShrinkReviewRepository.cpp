#include "MediaShrinkReviewRepository.h"

#include "Database.h"
#include "MutationOperation.h"
#include "MutationOperationRepository.h"

#include <sqlite3.h>
#include <algorithm>
#include <limits>
#include <string>

namespace vdrsuite::addons {
namespace {
using Status = MediaShrinkReviewStatus;
using Result = MediaShrinkReviewResult;
using vdrsuite::operations::MutationOperationState;
using vdrsuite::operations::MutationOperationVerificationPolicy;

bool opaque(const std::string& id, const char* prefix) {
    const std::string start(prefix);
    if (id.size() != start.size() + 32 ||
        id.compare(0, start.size(), start) != 0) return false;
    return std::all_of(id.begin() + start.size(), id.end(),
        [](unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
}
bool identity(const std::string& id) {
    return !id.empty() && id.size() <= 160 &&
        std::all_of(id.begin(), id.end(), [](unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z') || c == '_' || c == '-' ||
                   c == '.' || c == ':';
        });
}
Result output(Status s, const MediaShrinkReview& r = {}) {
    return {s, r};
}
bool bind(sqlite3_stmt* stmt, int col, const std::string& val) {
    return sqlite3_bind_text(stmt, col, val.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}
class Statement {
public:
    Statement(sqlite3* db, const char* query) {
        if (db && sqlite3_prepare_v2(db, query, -1, &value_, nullptr) != SQLITE_OK)
            value_ = nullptr;
    }
    ~Statement() { sqlite3_finalize(value_); }
    sqlite3_stmt* get() const { return value_; }
private:
    sqlite3_stmt* value_ = nullptr;
};
class Transaction {
public:
    explicit Transaction(Database& db)
        : db_(db), active_(db.execute("BEGIN IMMEDIATE;")) {}
    ~Transaction() { if (active_) db_.execute("ROLLBACK;"); }
    bool valid() const { return active_; }
    bool commit() {
        if (!active_ || !db_.execute("COMMIT;")) return false;
        active_ = false;
        return true;
    }
private:
    Database& db_;
    bool active_;
};

std::string field(sqlite3_stmt* q, int i) {
    const auto* text = sqlite3_column_text(q, i);
    return text ? reinterpret_cast<const char*>(text) : std::string();
}
Result read(Database& database, const std::string& actor,
            const std::string& backend, const std::string& id) {
    Statement q(database.handle(),
        "SELECT review_id,stage_operation_id,publish_operation_id,"
        "actor_id,backend_id,recording_id,backend_generation,"
        "source_fingerprint,state,revision,published_recording_id "
        "FROM addon_media_shrink_reviews WHERE review_id=? "
        "AND actor_id=? AND backend_id=?;");
    if (!q.get() || !bind(q.get(),1,id) ||
        !bind(q.get(),2,actor) || !bind(q.get(),3,backend))
        return output(Status::unavailable);
    const int step = sqlite3_step(q.get());
    if (step == SQLITE_DONE) return output(Status::notFound);
    if (step != SQLITE_ROW) return output(Status::unavailable);
    MediaShrinkReview review;
    review.reviewId = field(q.get(),0);
    review.stageOperationId = field(q.get(),1);
    review.publishOperationId = field(q.get(),2);
    review.actorId = field(q.get(),3);
    review.backendId = field(q.get(),4);
    review.recordingId = field(q.get(),5);
    const auto generation = sqlite3_column_int64(q.get(),6);
    review.sourceFingerprint = field(q.get(),7);
    review.state = field(q.get(),8);
    const auto revision = sqlite3_column_int64(q.get(),9);
    review.publishedRecordingId = field(q.get(),10);
    const bool staged = review.state == "staged" && revision == 1 &&
        review.publishOperationId.empty() &&
        review.publishedRecordingId.empty();
    const bool publishing = review.state == "publishing" && revision == 2 &&
        identity(review.publishOperationId) &&
        review.publishedRecordingId.empty();
    const bool published = review.state == "published" && revision == 3 &&
        identity(review.publishOperationId) &&
        opaque(review.publishedRecordingId, "rec_");
    // Rows that contradict the single-stage/single-publication lifecycle
    // cannot become executable again after corruption or a stale restart.
    if (generation <= 0 || !(staged || publishing || published) ||
        !opaque(review.reviewId, "shk_") ||
        !opaque(review.recordingId, "rec_") ||
        !identity(review.actorId) || !identity(review.backendId) ||
        !identity(review.stageOperationId) ||
        !identity(review.sourceFingerprint) ||
        sqlite3_step(q.get()) != SQLITE_DONE)
        return output(Status::unavailable);
    review.backendGeneration = static_cast<std::uint64_t>(generation);
    review.revision = static_cast<std::uint64_t>(revision);
    return output(Status::ok, review);
}
bool verifiedStage(const vdrsuite::operations::MutationOperation& op,
                   const std::string& reviewId) {
    return op.state == MutationOperationState::succeeded &&
           op.actionFamily == "media.shrink.stage" &&
           op.resourceType == "Recording" &&
           opaque(op.resourceId, "rec_") &&
           op.verificationPolicy ==
               MutationOperationVerificationPolicy::readbackRequired &&
           op.resultReference == reviewId &&
           op.backendGeneration > 0 &&
           op.backendGeneration <=
               static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) &&
           identity(op.operationId) && identity(op.actorId) &&
           identity(op.backendId) && op.backendId != "*" &&
           identity(op.expectedResourceFingerprint);
}
bool validPublish(const vdrsuite::operations::MutationOperation& op,
                  const MediaShrinkReview& review,
                  std::uint64_t revision) {
    return op.actionFamily == "media.shrink.publish" &&
           op.resourceType == "MediaShrinkReview" &&
           op.resourceId == review.reviewId &&
           op.actorId == review.actorId && op.backendId == review.backendId &&
           op.backendGeneration == review.backendGeneration &&
           op.expectedRevision == std::to_string(revision) &&
           op.expectedResourceFingerprint == review.sourceFingerprint &&
           op.verificationPolicy ==
               MutationOperationVerificationPolicy::reconciliationRequired;
}
} // namespace

MediaShrinkReviewRepository::MediaShrinkReviewRepository(
    Database& database,
    vdrsuite::operations::MutationOperationRepository& operations)
    : database_(database), operations_(operations) {}

bool MediaShrinkReviewRepository::ensureSchema() {
    auto lease = database_.acquireTransactionLease();
    if (operations_.databaseIdentity() != &database_) return false;
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS addon_media_shrink_reviews ("
        "review_id TEXT PRIMARY KEY,"
        "stage_operation_id TEXT NOT NULL UNIQUE,"
        "publish_operation_id TEXT NOT NULL DEFAULT '',"
        "actor_id TEXT NOT NULL,backend_id TEXT NOT NULL,"
        "recording_id TEXT NOT NULL,backend_generation INTEGER NOT NULL,"
        "source_fingerprint TEXT NOT NULL,"
        "state TEXT NOT NULL CHECK(state IN ('staged','publishing','published')),"
        "revision INTEGER NOT NULL CHECK(revision>=1),"
        "published_recording_id TEXT NOT NULL DEFAULT '');");
}

MediaShrinkReviewResult MediaShrinkReviewRepository::registerStaged(
    const std::string& reviewId, const std::string& stageOperationId) {
    if (!opaque(reviewId,"shk_") || !identity(stageOperationId))
        return output(Status::invalid);
    auto lease = database_.acquireTransactionLease();
    Transaction tx(database_);
    if (!tx.valid() || operations_.databaseIdentity() != &database_)
        return output(Status::unavailable);
    const auto staged = operations_.findById(stageOperationId);
    if (!staged.ok() || !verifiedStage(staged.operation, reviewId))
        return output(Status::conflict);
    const auto& op = staged.operation;
    auto existing = read(database_,op.actorId,op.backendId,reviewId);
    if (existing.ok()) {
        if (existing.review.stageOperationId != stageOperationId)
            return output(Status::conflict);
        return tx.commit() ? output(Status::replayed,existing.review)
                           : output(Status::unavailable);
    }
    if (existing.status != Status::notFound)
        return output(Status::unavailable);
    Statement insert(database_.handle(),
        "INSERT INTO addon_media_shrink_reviews("
        "review_id,stage_operation_id,actor_id,backend_id,recording_id,"
        "backend_generation,source_fingerprint,state,revision) "
        "VALUES(?,?,?,?,?,?,?,'staged',1);");
    if (!insert.get() || !bind(insert.get(),1,reviewId) ||
        !bind(insert.get(),2,stageOperationId) ||
        !bind(insert.get(),3,op.actorId) ||
        !bind(insert.get(),4,op.backendId) ||
        !bind(insert.get(),5,op.resourceId) ||
        sqlite3_bind_int64(insert.get(),6,
            static_cast<sqlite3_int64>(op.backendGeneration)) != SQLITE_OK ||
        !bind(insert.get(),7,op.expectedResourceFingerprint) ||
        sqlite3_step(insert.get()) != SQLITE_DONE ||
        sqlite3_changes(database_.handle()) != 1)
        return output(Status::conflict);
    if (!tx.commit()) return output(Status::unavailable);
    return find(op.actorId,op.backendId,reviewId);
}

MediaShrinkReviewResult MediaShrinkReviewRepository::find(
    const std::string& actor, const std::string& backend,
    const std::string& reviewId) const {
    if (!identity(actor) || !identity(backend) ||
        !opaque(reviewId,"shk_")) return output(Status::invalid);
    auto lease = database_.acquireTransactionLease();
    return read(database_,actor,backend,reviewId);
}

MediaShrinkReviewResult MediaShrinkReviewRepository::claimPublication(
    const std::string& actor, const std::string& backend,
    const std::string& reviewId, std::uint64_t expectedRevision,
    const std::string& publishOperationId) {
    if (!identity(actor) || !identity(backend) ||
        !opaque(reviewId,"shk_") || !identity(publishOperationId) ||
        expectedRevision == 0 ||
        expectedRevision >=
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return output(Status::invalid);
    auto lease = database_.acquireTransactionLease();
    Transaction tx(database_);
    if (!tx.valid()) return output(Status::unavailable);
    auto selected = read(database_,actor,backend,reviewId);
    if (!selected.ok()) return selected;
    const auto& review = selected.review;
    if (review.state == "publishing" &&
        review.publishOperationId == publishOperationId &&
        review.revision == expectedRevision + 1) {
        // A replay is informative only. In particular, a finished or
        // outcome-unknown command must never be reissued after a restart.
        const auto original = operations_.findById(publishOperationId);
        if (!original.ok() ||
            !validPublish(original.operation, review, expectedRevision) ||
            (original.operation.state != MutationOperationState::accepted &&
             original.operation.state != MutationOperationState::dispatching))
            return output(Status::conflict);
        return tx.commit() ? output(Status::replayed,review)
                           : output(Status::unavailable);
    }
    if (review.state != "staged" || review.revision != expectedRevision)
        return output(Status::conflict);
    const auto publish = operations_.findById(publishOperationId);
    if (!publish.ok() ||
        publish.operation.state != MutationOperationState::accepted ||
        !validPublish(publish.operation,review,expectedRevision))
        return output(Status::conflict);
    Statement update(database_.handle(),
        "UPDATE addon_media_shrink_reviews SET "
        "state='publishing',publish_operation_id=?,revision=revision+1 "
        "WHERE review_id=? AND actor_id=? AND backend_id=? "
        "AND state='staged' AND revision=?;");
    if (!update.get() || !bind(update.get(),1,publishOperationId) ||
        !bind(update.get(),2,reviewId) || !bind(update.get(),3,actor) ||
        !bind(update.get(),4,backend) ||
        sqlite3_bind_int64(update.get(),5,
            static_cast<sqlite3_int64>(expectedRevision)) != SQLITE_OK ||
        sqlite3_step(update.get()) != SQLITE_DONE ||
        sqlite3_changes(database_.handle()) != 1)
        return output(Status::unavailable);
    if (!tx.commit()) return output(Status::unavailable);
    return find(actor,backend,reviewId);
}

MediaShrinkReviewResult MediaShrinkReviewRepository::completePublication(
    const std::string& actor, const std::string& backend,
    const std::string& reviewId, std::uint64_t expectedRevision) {
    if (!identity(actor) || !identity(backend) ||
        !opaque(reviewId,"shk_") || expectedRevision == 0 ||
        expectedRevision >=
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return output(Status::invalid);
    auto lease = database_.acquireTransactionLease();
    Transaction tx(database_);
    if (!tx.valid()) return output(Status::unavailable);
    auto selected = read(database_,actor,backend,reviewId);
    if (!selected.ok()) return selected;
    const auto& review = selected.review;
    if (review.state != "publishing" ||
        review.revision != expectedRevision ||
        !identity(review.publishOperationId))
        return output(Status::conflict);
    const auto publish = operations_.findById(review.publishOperationId);
    if (!publish.ok() ||
        publish.operation.state != MutationOperationState::succeeded ||
        !validPublish(publish.operation,review,expectedRevision-1) ||
        !opaque(publish.operation.resultReference,"rec_"))
        return output(Status::conflict);
    Statement update(database_.handle(),
        "UPDATE addon_media_shrink_reviews SET "
        "state='published',published_recording_id=?,revision=revision+1 "
        "WHERE review_id=? AND actor_id=? AND backend_id=? "
        "AND state='publishing' AND revision=?;");
    if (!update.get() ||
        !bind(update.get(),1,publish.operation.resultReference) ||
        !bind(update.get(),2,reviewId) || !bind(update.get(),3,actor) ||
        !bind(update.get(),4,backend) ||
        sqlite3_bind_int64(update.get(),5,
            static_cast<sqlite3_int64>(expectedRevision)) != SQLITE_OK ||
        sqlite3_step(update.get()) != SQLITE_DONE ||
        sqlite3_changes(database_.handle()) != 1)
        return output(Status::unavailable);
    if (!tx.commit()) return output(Status::unavailable);
    return find(actor,backend,reviewId);
}
} // namespace vdrsuite::addons
