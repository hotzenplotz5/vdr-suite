#include "TimerAssignmentRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxReasonLength = 256;

bool safeIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    return sqlite3_bind_text(
        statement,
        index,
        value.data(),
        static_cast<int>(value.size()),
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool parseUnsignedToken(const std::string& token, sqlite3_int64& value)
{
    if (token.empty()) return false;
    value = 0;
    for (char ch : token)
    {
        if (ch < '0' || ch > '9') return false;
        const sqlite3_int64 digit = ch - '0';
        if (value > (std::numeric_limits<sqlite3_int64>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    return true;
}

const char* outcomeName(TimerAssignmentReassignmentNativeOutcome outcome)
{
    return outcome == TimerAssignmentReassignmentNativeOutcome::beforeDispatch
        ? "before_dispatch"
        : "verified_absent";
}

bool parseOutcome(
    const std::string& value,
    TimerAssignmentReassignmentNativeOutcome& outcome)
{
    if (value == "before_dispatch")
        outcome = TimerAssignmentReassignmentNativeOutcome::beforeDispatch;
    else if (value == "verified_absent")
        outcome = TimerAssignmentReassignmentNativeOutcome::verifiedAbsent;
    else
        return false;
    return true;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string()
        : std::string(reinterpret_cast<const char*>(value));
}

bool hasColumn(
    Database& database,
    const std::string& table,
    const std::string& column)
{
    sqlite3_stmt* statement = nullptr;
    const std::string sql = "PRAGMA table_info(" + table + ");";
    if (sqlite3_prepare_v2(
            database.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK)
        return false;

    bool found = false;
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        if (columnText(statement, 1) == column)
        {
            found = true;
            break;
        }
    }
    sqlite3_finalize(statement);
    return found;
}

bool evidenceShapeValid(const TimerAssignmentReassignmentEvidence& evidence)
{
    if (!safeIdentity(evidence.oldTimerAssignmentId)
        || !safeIdentity(evidence.oldAssignmentRevision)
        || evidence.oldAssignmentEpoch == 0
        || !safeIdentity(evidence.oldBackendId)
        || evidence.oldBackendGeneration == 0
        || !safeIdentity(evidence.replacementTimerAssignmentId)
        || !safeIdentity(evidence.newBackendId)
        || evidence.newBackendGeneration == 0
        || evidence.newAssignmentEpoch != 0
        || evidence.reason.empty()
        || evidence.reason.size() > kMaxReasonLength
        || evidence.createdAt <= 0)
    {
        return false;
    }

    if (evidence.oldNativeOutcome
        == TimerAssignmentReassignmentNativeOutcome::beforeDispatch)
    {
        return evidence.oldOperationId.empty()
            && evidence.oldOperationRevision.empty()
            && evidence.oldNativeTimerBindingId.empty()
            && evidence.oldBindingRevision.empty();
    }

    return safeIdentity(evidence.oldOperationId)
        && safeIdentity(evidence.oldOperationRevision)
        && safeIdentity(evidence.oldNativeTimerBindingId)
        && safeIdentity(evidence.oldBindingRevision);
}

bool ensureReassignmentSchema(Database& database)
{
    if (!database.execute(
        "CREATE TABLE IF NOT EXISTS timer_assignment_reassignments ("
        "replacement_assignment_id TEXT PRIMARY KEY NOT NULL,"
        "old_assignment_id TEXT NOT NULL UNIQUE,"
        "old_assignment_revision TEXT NOT NULL,"
        "old_assignment_epoch INTEGER NOT NULL CHECK(old_assignment_epoch > 0),"
        "old_backend_id TEXT NOT NULL,"
        "old_backend_generation INTEGER NOT NULL CHECK(old_backend_generation > 0),"
        "old_native_outcome TEXT NOT NULL CHECK(old_native_outcome IN "
        "('before_dispatch','verified_absent')),"
        "old_operation_id TEXT NOT NULL DEFAULT '',"
        "old_operation_revision TEXT NOT NULL DEFAULT '',"
        "old_binding_id TEXT NOT NULL DEFAULT '',"
        "old_binding_revision TEXT NOT NULL DEFAULT '',"
        "reason TEXT NOT NULL,"
        "new_backend_id TEXT NOT NULL,"
        "new_backend_generation INTEGER NOT NULL CHECK(new_backend_generation > 0),"
        "new_assignment_epoch INTEGER NOT NULL CHECK(new_assignment_epoch > 0),"
        "created_at INTEGER NOT NULL CHECK(created_at > 0),"
        "FOREIGN KEY(old_assignment_id) REFERENCES timer_assignments(timer_assignment_id),"
        "FOREIGN KEY(replacement_assignment_id) REFERENCES timer_assignments(timer_assignment_id)"
        ");"))
        return false;

    return hasColumn(
               database,
               "timer_assignment_reassignments",
               "old_operation_revision")
        || database.execute(
            "ALTER TABLE timer_assignment_reassignments "
            "ADD COLUMN old_operation_revision TEXT NOT NULL DEFAULT '';");
}

void removeExpectation(Database& database)
{
    database.execute(
        "DROP TRIGGER IF EXISTS temp.trg_timer_assignment_reassignment_before;"
        "DROP TRIGGER IF EXISTS temp.trg_timer_assignment_reassignment_after;"
        "DROP TABLE IF EXISTS temp.timer_assignment_reassignment_expectation;");
}

bool installExpectation(
    Database& database,
    const std::string& expectedSetRevision,
    const TimerAssignmentReassignmentEvidence& evidence)
{
    sqlite3_int64 setRevision = 0;
    sqlite3_int64 oldRevision = 0;
    if (!parseUnsignedToken(expectedSetRevision, setRevision)
        || !parseUnsignedToken(evidence.oldAssignmentRevision, oldRevision))
    {
        return false;
    }

    removeExpectation(database);
    if (!database.execute(
            "CREATE TEMP TABLE timer_assignment_reassignment_expectation ("
            "replacement_assignment_id TEXT PRIMARY KEY NOT NULL,"
            "expected_set_revision INTEGER NOT NULL,"
            "old_assignment_id TEXT NOT NULL,"
            "old_assignment_revision INTEGER NOT NULL,"
            "old_assignment_epoch INTEGER NOT NULL,"
            "old_backend_id TEXT NOT NULL,"
            "old_backend_generation INTEGER NOT NULL,"
            "old_native_outcome TEXT NOT NULL,"
            "old_operation_id TEXT NOT NULL,"
            "old_operation_revision TEXT NOT NULL,"
            "old_binding_id TEXT NOT NULL,"
            "old_binding_revision TEXT NOT NULL,"
            "reason TEXT NOT NULL,"
            "created_at INTEGER NOT NULL"
            ");"))
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO temp.timer_assignment_reassignment_expectation VALUES "
        "(?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(database.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK)
    {
        removeExpectation(database);
        return false;
    }

    const bool bound =
        bindText(statement, 1, evidence.replacementTimerAssignmentId)
        && sqlite3_bind_int64(statement, 2, setRevision) == SQLITE_OK
        && bindText(statement, 3, evidence.oldTimerAssignmentId)
        && sqlite3_bind_int64(statement, 4, oldRevision) == SQLITE_OK
        && sqlite3_bind_int64(
            statement, 5, static_cast<sqlite3_int64>(evidence.oldAssignmentEpoch))
            == SQLITE_OK
        && bindText(statement, 6, evidence.oldBackendId)
        && sqlite3_bind_int64(
            statement, 7, static_cast<sqlite3_int64>(evidence.oldBackendGeneration))
            == SQLITE_OK
        && bindText(statement, 8, outcomeName(evidence.oldNativeOutcome))
        && bindText(statement, 9, evidence.oldOperationId)
        && bindText(statement, 10, evidence.oldOperationRevision)
        && bindText(statement, 11, evidence.oldNativeTimerBindingId)
        && bindText(statement, 12, evidence.oldBindingRevision)
        && bindText(statement, 13, evidence.reason)
        && sqlite3_bind_int64(statement, 14, evidence.createdAt) == SQLITE_OK;
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    sqlite3_finalize(statement);
    if (step != SQLITE_DONE)
    {
        removeExpectation(database);
        return false;
    }

    if (!database.execute(
            "CREATE TEMP TRIGGER trg_timer_assignment_reassignment_before "
            "BEFORE INSERT ON main.timer_assignments "
            "WHEN NEW.timer_assignment_id=(SELECT replacement_assignment_id "
            "FROM timer_assignment_reassignment_expectation LIMIT 1) "
            "BEGIN "
            "SELECT CASE WHEN NEW.role!='replacement' OR NEW.state!='selected' "
            "THEN RAISE(ABORT,'replacement_shape_conflict') END;"
            "SELECT CASE WHEN NEW.backend_id=(SELECT old_backend_id FROM "
            "timer_assignment_reassignment_expectation LIMIT 1) "
            "THEN RAISE(ABORT,'replacement_backend_conflict') END;"
            "SELECT CASE WHEN COALESCE((SELECT set_revision FROM "
            "timer_assignment_set_revisions WHERE timer_intent_id=NEW.timer_intent_id),0)"
            "!=(SELECT expected_set_revision FROM "
            "timer_assignment_reassignment_expectation LIMIT 1) "
            "THEN RAISE(ABORT,'assignment_set_conflict') END;"
            "SELECT CASE WHEN NOT EXISTS(SELECT 1 FROM timer_assignments old "
            "JOIN timer_assignment_reassignment_expectation e "
            "ON e.old_assignment_id=old.timer_assignment_id "
            "WHERE old.timer_intent_id=NEW.timer_intent_id "
            "AND old.assignment_revision=e.old_assignment_revision "
            "AND old.assignment_epoch=e.old_assignment_epoch "
            "AND old.backend_id=e.old_backend_id "
            "AND old.backend_generation=e.old_backend_generation "
            "AND old.role IN ('primary','replacement') "
            "AND old.state IN ('selected','provisioning','bound','reconciling') "
            "AND ((e.old_native_outcome='before_dispatch' "
            "AND old.state='selected' AND old.native_timer_binding_id='') "
            "OR (e.old_native_outcome='verified_absent' "
            "AND old.state IN ('bound','reconciling') "
            "AND old.native_timer_binding_id=e.old_binding_id)) "
            "AND NEW.created_at>old.updated_at) "
            "THEN RAISE(ABORT,'old_owner_conflict') END;"
            "SELECT CASE WHEN NOT EXISTS(SELECT 1 FROM timer_intents intent "
            "WHERE intent.timer_intent_id=NEW.timer_intent_id "
            "AND CAST(intent.intent_revision AS TEXT)=NEW.intent_revision "
            "AND intent.assignment_allow_failover=1) "
            "THEN RAISE(ABORT,'intent_conflict') END;"
            "SELECT CASE WHEN (SELECT old_native_outcome FROM "
            "timer_assignment_reassignment_expectation LIMIT 1)='verified_absent' "
            "AND NOT EXISTS(SELECT 1 FROM native_timer_bindings binding "
            "JOIN timer_assignment_reassignment_expectation e "
            "ON e.old_binding_id=binding.native_timer_binding_id "
            "WHERE CAST(binding.binding_revision AS TEXT)=e.old_binding_revision "
            "AND binding.timer_assignment_id=e.old_assignment_id "
            "AND binding.backend_id=e.old_backend_id "
            "AND binding.backend_generation=e.old_backend_generation "
            "AND binding.ownership IN ('managed','adopted') "
            "AND binding.missing_since>0 "
            "AND binding.drift_state='expected_transition' "
            "AND binding.observed_recording=0 "
            "AND binding.last_verified_operation_id=e.old_operation_id) "
            "THEN RAISE(ABORT,'binding_conflict') END;"
            "SELECT CASE WHEN (SELECT old_native_outcome FROM "
            "timer_assignment_reassignment_expectation LIMIT 1)='verified_absent' "
            "AND NOT EXISTS(SELECT 1 FROM mutation_operations operation "
            "JOIN timer_assignment_reassignment_expectation e "
            "ON e.old_operation_id=operation.operation_id "
            "WHERE CAST(operation.operation_revision AS TEXT)=e.old_operation_revision "
            "AND operation.state='succeeded' "
            "AND operation.action_family='timer.delete' "
            "AND operation.resource_type='NativeTimerBinding' "
            "AND operation.resource_id=e.old_binding_id "
            "AND operation.backend_id=e.old_backend_id "
            "AND operation.backend_generation=e.old_backend_generation) "
            "THEN RAISE(ABORT,'operation_conflict') END;"
            "UPDATE timer_assignments SET state='superseded',"
            "assignment_revision=assignment_revision+1,updated_at=NEW.created_at "
            "WHERE timer_assignment_id=(SELECT old_assignment_id FROM "
            "timer_assignment_reassignment_expectation LIMIT 1);"
            "END;"
            "CREATE TEMP TRIGGER trg_timer_assignment_reassignment_after "
            "AFTER INSERT ON main.timer_assignments "
            "WHEN NEW.timer_assignment_id=(SELECT replacement_assignment_id "
            "FROM timer_assignment_reassignment_expectation LIMIT 1) "
            "BEGIN "
            "INSERT INTO timer_assignment_reassignments ("
            "replacement_assignment_id,old_assignment_id,old_assignment_revision,"
            "old_assignment_epoch,old_backend_id,old_backend_generation,"
            "old_native_outcome,old_operation_id,old_operation_revision,old_binding_id,old_binding_revision,"
            "reason,new_backend_id,new_backend_generation,new_assignment_epoch,created_at"
            ") SELECT NEW.timer_assignment_id,e.old_assignment_id,"
            "CAST(e.old_assignment_revision AS TEXT),e.old_assignment_epoch,"
            "e.old_backend_id,e.old_backend_generation,e.old_native_outcome,"
            "e.old_operation_id,e.old_operation_revision,e.old_binding_id,e.old_binding_revision,e.reason,"
            "NEW.backend_id,NEW.backend_generation,NEW.assignment_epoch,e.created_at "
            "FROM timer_assignment_reassignment_expectation e;"
            "END;"))
    {
        removeExpectation(database);
        return false;
    }
    return true;
}

TimerAssignmentControlledReplacementResult statusResult(
    TimerAssignmentRepositoryStatus status)
{
    TimerAssignmentControlledReplacementResult result;
    result.status = status;
    return result;
}
} // namespace

TimerAssignmentControlledReplacementResult
TimerAssignmentRepository::createControlledReplacement(
    const TimerAssignment& replacement,
    const std::string& expectedAssignmentSetRevision,
    const TimerAssignmentReassignmentEvidence& evidence)
{
    if (replacement.timerAssignmentId != evidence.replacementTimerAssignmentId
        || replacement.backendId != evidence.newBackendId
        || replacement.backendGeneration != evidence.newBackendGeneration
        || replacement.createdAt != evidence.createdAt
        || replacement.role != TimerAssignmentRole::replacement
        || replacement.state != TimerAssignmentState::selected
        || !evidenceShapeValid(evidence))
    {
        return statusResult(TimerAssignmentRepositoryStatus::invalid);
    }

    auto lease = database_.acquireTransactionLease();
    const auto currentSet = assignmentSetRevisionForIntent(replacement.timerIntentId);
    if (!currentSet.ok()) return statusResult(currentSet.status);
    if (currentSet.assignmentSetRevision != expectedAssignmentSetRevision)
        return statusResult(TimerAssignmentRepositoryStatus::conflict);
    if (!ensureReassignmentSchema(database_)
        || !installExpectation(database_, expectedAssignmentSetRevision, evidence))
    {
        return statusResult(TimerAssignmentRepositoryStatus::storageError);
    }

    TimerAssignmentRepositoryResult created = create(replacement);
    removeExpectation(database_);
    if (!created.ok())
    {
        const auto afterFailure = assignmentSetRevisionForIntent(
            replacement.timerIntentId);
        if (afterFailure.ok()
            && afterFailure.assignmentSetRevision
                != expectedAssignmentSetRevision)
        {
            return statusResult(TimerAssignmentRepositoryStatus::conflict);
        }
        return statusResult(created.status);
    }

    auto result = findControlledReplacement(replacement.timerAssignmentId);
    if (!result.ok()) return result;
    return result;
}

TimerAssignmentControlledReplacementResult
TimerAssignmentRepository::findControlledReplacement(
    const std::string& replacementTimerAssignmentId)
{
    if (!safeIdentity(replacementTimerAssignmentId))
        return statusResult(TimerAssignmentRepositoryStatus::invalid);

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema() || !ensureReassignmentSchema(database_))
        return statusResult(TimerAssignmentRepositoryStatus::storageError);

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT old_assignment_id,old_assignment_revision,old_assignment_epoch,"
        "old_backend_id,old_backend_generation,old_native_outcome,old_operation_id,"
        "old_operation_revision,old_binding_id,old_binding_revision,reason,new_backend_id,"
        "new_backend_generation,new_assignment_epoch,created_at "
        "FROM timer_assignment_reassignments WHERE replacement_assignment_id=?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK || !bindText(statement, 1, replacementTimerAssignmentId))
    {
        if (statement) sqlite3_finalize(statement);
        return statusResult(TimerAssignmentRepositoryStatus::storageError);
    }

    const int step = sqlite3_step(statement);
    if (step == SQLITE_DONE)
    {
        sqlite3_finalize(statement);
        return statusResult(TimerAssignmentRepositoryStatus::notFound);
    }
    if (step != SQLITE_ROW)
    {
        sqlite3_finalize(statement);
        return statusResult(TimerAssignmentRepositoryStatus::storageError);
    }

    TimerAssignmentReassignmentEvidence evidence;
    const auto text = [&](int column) -> std::string {
        const unsigned char* value = sqlite3_column_text(statement, column);
        return value ? reinterpret_cast<const char*>(value) : std::string();
    };
    evidence.oldTimerAssignmentId = text(0);
    evidence.oldAssignmentRevision = text(1);
    evidence.oldAssignmentEpoch = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 2));
    evidence.oldBackendId = text(3);
    evidence.oldBackendGeneration = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 4));
    const bool outcomeOk = parseOutcome(text(5), evidence.oldNativeOutcome);
    evidence.oldOperationId = text(6);
    evidence.oldOperationRevision = text(7);
    evidence.oldNativeTimerBindingId = text(8);
    evidence.oldBindingRevision = text(9);
    evidence.reason = text(10);
    evidence.replacementTimerAssignmentId = replacementTimerAssignmentId;
    evidence.newBackendId = text(11);
    evidence.newBackendGeneration = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 12));
    evidence.newAssignmentEpoch = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 13));
    evidence.createdAt = sqlite3_column_int64(statement, 14);
    sqlite3_finalize(statement);
    if (!outcomeOk) return statusResult(TimerAssignmentRepositoryStatus::storageError);

    const auto oldAssignment = findById(evidence.oldTimerAssignmentId);
    const auto replacement = findById(replacementTimerAssignmentId);
    if (!oldAssignment.ok() || !replacement.ok())
        return statusResult(TimerAssignmentRepositoryStatus::storageError);

    TimerAssignmentControlledReplacementResult result;
    result.status = TimerAssignmentRepositoryStatus::ok;
    result.oldAssignment = oldAssignment.assignment;
    result.replacementAssignment = replacement.assignment;
    result.evidence = evidence;
    return result;
}

} // namespace vdrsuite::timers
