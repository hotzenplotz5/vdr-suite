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

bool safeIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

bool bindText(
    sqlite3_stmt* statement,
    int index,
    const std::string& value)
{
    if (value.size() >
        static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        return false;
    }

    return sqlite3_bind_text(
        statement,
        index,
        value.data(),
        static_cast<int>(value.size()),
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool parseSetRevisionToken(
    const std::string& token,
    sqlite3_int64& revision)
{
    if (token.empty())
    {
        return false;
    }

    sqlite3_int64 value = 0;
    for (char ch : token)
    {
        if (ch < '0' || ch > '9')
        {
            return false;
        }

        const sqlite3_int64 digit = ch - '0';
        if (value >
            (std::numeric_limits<sqlite3_int64>::max() - digit) / 10)
        {
            return false;
        }
        value = value * 10 + digit;
    }

    revision = value;
    return true;
}

bool ensureAssignmentSetRevisionSchema(Database& database)
{
    return database.execute(
        "CREATE TABLE IF NOT EXISTS timer_assignment_set_revisions ("
        "timer_intent_id TEXT PRIMARY KEY NOT NULL,"
        "set_revision INTEGER NOT NULL CHECK(set_revision >= 0),"
        "FOREIGN KEY(timer_intent_id) "
        "REFERENCES timer_intents(timer_intent_id)"
        ");"
        "INSERT OR IGNORE INTO timer_assignment_set_revisions "
        "(timer_intent_id,set_revision) "
        "SELECT timer_intent_id,COALESCE(MAX(assignment_epoch),0) "
        "FROM timer_assignments GROUP BY timer_intent_id;"
        "CREATE TRIGGER IF NOT EXISTS "
        "trg_timer_assignment_set_revision_insert "
        "AFTER INSERT ON timer_assignments "
        "BEGIN "
        "INSERT INTO timer_assignment_set_revisions "
        "(timer_intent_id,set_revision) "
        "VALUES (NEW.timer_intent_id,1) "
        "ON CONFLICT(timer_intent_id) DO UPDATE SET "
        "set_revision=set_revision+1;"
        "END;"
        "CREATE TRIGGER IF NOT EXISTS "
        "trg_timer_assignment_set_revision_update "
        "AFTER UPDATE ON timer_assignments "
        "BEGIN "
        "INSERT INTO timer_assignment_set_revisions "
        "(timer_intent_id,set_revision) "
        "VALUES (NEW.timer_intent_id,1) "
        "ON CONFLICT(timer_intent_id) DO UPDATE SET "
        "set_revision=set_revision+1;"
        "END;"
        "CREATE TRIGGER IF NOT EXISTS "
        "trg_timer_assignment_set_revision_delete "
        "AFTER DELETE ON timer_assignments "
        "BEGIN "
        "INSERT INTO timer_assignment_set_revisions "
        "(timer_intent_id,set_revision) "
        "VALUES (OLD.timer_intent_id,1) "
        "ON CONFLICT(timer_intent_id) DO UPDATE SET "
        "set_revision=set_revision+1;"
        "END;");
}

void removeTemporaryExpectationFence(Database& database)
{
    database.execute(
        "DROP TRIGGER IF EXISTS temp.trg_timer_assignment_set_expectation_insert;"
        "DROP TABLE IF EXISTS temp.timer_assignment_set_expectation;");
}

bool installTemporaryExpectationFence(
    Database& database,
    const TimerAssignment& assignment,
    sqlite3_int64 expectedRevision)
{
    removeTemporaryExpectationFence(database);

    if (!database.execute(
            "CREATE TEMP TABLE timer_assignment_set_expectation ("
            "timer_assignment_id TEXT PRIMARY KEY NOT NULL,"
            "timer_intent_id TEXT NOT NULL,"
            "expected_set_revision INTEGER NOT NULL "
            "CHECK(expected_set_revision >= 0)"
            ");"))
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO temp.timer_assignment_set_expectation "
        "(timer_assignment_id,timer_intent_id,expected_set_revision) "
        "VALUES (?,?,?);";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        removeTemporaryExpectationFence(database);
        return false;
    }

    const bool bound =
        bindText(statement, 1, assignment.timerAssignmentId)
        && bindText(statement, 2, assignment.timerIntentId)
        && sqlite3_bind_int64(
            statement,
            3,
            expectedRevision) == SQLITE_OK;
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
    {
        removeTemporaryExpectationFence(database);
        return false;
    }

    if (!database.execute(
            "CREATE TEMP TRIGGER trg_timer_assignment_set_expectation_insert "
            "BEFORE INSERT ON main.timer_assignments "
            "WHEN NEW.timer_assignment_id=("
            "SELECT timer_assignment_id "
            "FROM timer_assignment_set_expectation LIMIT 1"
            ") "
            "BEGIN "
            "SELECT CASE WHEN NOT EXISTS ("
            "SELECT 1 FROM timer_assignment_set_expectation e "
            "WHERE e.timer_assignment_id=NEW.timer_assignment_id "
            "AND e.timer_intent_id=NEW.timer_intent_id "
            "AND e.expected_set_revision=COALESCE(("
            "SELECT r.set_revision "
            "FROM timer_assignment_set_revisions r "
            "WHERE r.timer_intent_id=NEW.timer_intent_id"
            "),0)"
            ") THEN RAISE(ABORT,'timer_assignment_set_revision_conflict') END;"
            "END;"))
    {
        removeTemporaryExpectationFence(database);
        return false;
    }

    return true;
}

enum class IntentPresence
{
    present,
    missing,
    storageError,
};

IntentPresence intentPresence(
    Database& database,
    const std::string& timerIntentId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT 1 FROM timer_intents WHERE timer_intent_id=? LIMIT 1;";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return IntentPresence::storageError;
    }

    if (!bindText(statement, 1, timerIntentId))
    {
        sqlite3_finalize(statement);
        return IntentPresence::storageError;
    }

    const int step = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (step == SQLITE_ROW) return IntentPresence::present;
    if (step == SQLITE_DONE) return IntentPresence::missing;
    return IntentPresence::storageError;
}

bool readCurrentSetRevision(
    Database& database,
    const std::string& timerIntentId,
    sqlite3_int64& revision)
{
    revision = 0;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT COALESCE(("
        "SELECT set_revision FROM timer_assignment_set_revisions "
        "WHERE timer_intent_id=?"
        "),0);";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    if (!bindText(statement, 1, timerIntentId))
    {
        sqlite3_finalize(statement);
        return false;
    }

    const int step = sqlite3_step(statement);
    if (step != SQLITE_ROW)
    {
        sqlite3_finalize(statement);
        return false;
    }

    revision = sqlite3_column_int64(statement, 0);
    sqlite3_finalize(statement);
    return revision >= 0;
}


TimerAssignmentRepositorySetRevisionResult setRevisionResult(
    TimerAssignmentRepositoryStatus status)
{
    TimerAssignmentRepositorySetRevisionResult result;
    result.status = status;
    return result;
}

TimerAssignmentRepositorySetRevisionResult setRevisionResult(
    TimerAssignmentRepositoryStatus status,
    sqlite3_int64 revision)
{
    TimerAssignmentRepositorySetRevisionResult result;
    result.status = status;
    if (revision >= 0)
    {
        result.assignmentSetRevision = std::to_string(revision);
    }
    return result;
}

} // namespace

TimerAssignmentRepositorySetRevisionResult
TimerAssignmentRepository::assignmentSetRevisionForIntent(
    const std::string& timerIntentId)
{
    if (!safeIdentity(timerIntentId))
    {
        return setRevisionResult(
            TimerAssignmentRepositoryStatus::invalid);
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema()
        || !ensureAssignmentSetRevisionSchema(database_))
    {
        return setRevisionResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    const IntentPresence presence =
        intentPresence(database_, timerIntentId);
    if (presence == IntentPresence::missing)
    {
        return setRevisionResult(
            TimerAssignmentRepositoryStatus::intentNotFound);
    }
    if (presence == IntentPresence::storageError)
    {
        return setRevisionResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    sqlite3_int64 revision = 0;
    if (!readCurrentSetRevision(
            database_,
            timerIntentId,
            revision))
    {
        return setRevisionResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    return setRevisionResult(
        TimerAssignmentRepositoryStatus::ok,
        revision);
}

TimerAssignmentRepositoryResult
TimerAssignmentRepository::createAgainstAssignmentSetRevision(
    const TimerAssignment& assignment,
    const std::string& expectedAssignmentSetRevision)
{
    sqlite3_int64 expectedRevision = 0;
    if (!safeIdentity(assignment.timerAssignmentId)
        || !safeIdentity(assignment.timerIntentId)
        || !parseSetRevisionToken(
            expectedAssignmentSetRevision,
            expectedRevision))
    {
        TimerAssignmentRepositoryResult result;
        result.status = TimerAssignmentRepositoryStatus::invalid;
        return result;
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema()
        || !ensureAssignmentSetRevisionSchema(database_))
    {
        TimerAssignmentRepositoryResult result;
        result.status = TimerAssignmentRepositoryStatus::storageError;
        return result;
    }

    if (!installTemporaryExpectationFence(
            database_,
            assignment,
            expectedRevision))
    {
        TimerAssignmentRepositoryResult result;
        result.status = TimerAssignmentRepositoryStatus::storageError;
        return result;
    }

    TimerAssignmentRepositoryResult result = create(assignment);

    sqlite3_int64 currentRevision = 0;
    const bool revisionReadable =
        readCurrentSetRevision(
            database_,
            assignment.timerIntentId,
            currentRevision);

    removeTemporaryExpectationFence(database_);

    if (!result.ok()
        && revisionReadable
        && currentRevision != expectedRevision
        && (result.status
            == TimerAssignmentRepositoryStatus::ownershipConflict
            || result.status
            == TimerAssignmentRepositoryStatus::storageError))
    {
        result.status =
            TimerAssignmentRepositoryStatus::conflict;
    }

    return result;
}

}
