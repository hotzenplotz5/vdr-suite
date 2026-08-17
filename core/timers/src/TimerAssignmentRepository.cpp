#include "TimerAssignmentRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace vdrsuite::timers
{
namespace
{

constexpr std::size_t kMaxIdentityLength = 160;

const char* kSelectColumns =
    "timer_assignment_id,assignment_revision,timer_intent_id,intent_revision,"
    "assignment_epoch,backend_id,backend_generation,state,role,"
    "channel_canonical_id,channel_backend_id,channel_mapping_source,"
    "channel_mapping_revision,capability_revision,backend_health_revision,"
    "decision_policy_version,decision_reasons,decision_warnings,"
    "decision_exclusions,decision_conflict_facts,decision_score,"
    "native_timer_binding_id,created_at,updated_at";

bool safeIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value);

bool controlledReplacementExpected(
    Database& database,
    const std::string& timerAssignmentId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT 1 FROM sqlite_temp_master "
        "WHERE type='table' AND name='timer_assignment_reassignment_expectation' "
        "AND EXISTS(SELECT 1 FROM temp.timer_assignment_reassignment_expectation "
        "WHERE replacement_assignment_id=?);";
    if (sqlite3_prepare_v2(database.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK)
    {
        return false;
    }
    const bool bound = bindText(statement, 1, timerAssignmentId);
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    sqlite3_finalize(statement);
    return step == SQLITE_ROW;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* text = sqlite3_column_text(statement, column);
    if (!text)
    {
        return {};
    }

    const int bytes = sqlite3_column_bytes(statement, column);
    return std::string(
        reinterpret_cast<const char*>(text),
        static_cast<std::size_t>(bytes));
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

bool parseRevisionToken(
    const std::string& token,
    std::int64_t& revision)
{
    if (token.empty())
    {
        return false;
    }

    std::int64_t value = 0;
    for (char ch : token)
    {
        if (ch < '0' || ch > '9')
        {
            return false;
        }

        const std::int64_t digit = ch - '0';
        if (value >
            (std::numeric_limits<std::int64_t>::max() - digit) / 10)
        {
            return false;
        }
        value = value * 10 + digit;
    }

    if (value <= 0)
    {
        return false;
    }

    revision = value;
    return true;
}

std::string encodeStrings(const std::vector<std::string>& values)
{
    std::string encoded;
    for (const auto& value : values)
    {
        encoded.append(std::to_string(value.size()));
        encoded.push_back(':');
        encoded.append(value);
    }
    return encoded;
}

bool decodeStrings(
    const std::string& encoded,
    std::vector<std::string>& values)
{
    values.clear();
    std::size_t position = 0;

    while (position < encoded.size())
    {
        const std::size_t colon = encoded.find(':', position);
        if (colon == std::string::npos || colon == position)
        {
            return false;
        }

        std::size_t length = 0;
        for (std::size_t index = position; index < colon; ++index)
        {
            const char ch = encoded[index];
            if (ch < '0' || ch > '9')
            {
                return false;
            }

            const std::size_t digit =
                static_cast<std::size_t>(ch - '0');
            if (length >
                (std::numeric_limits<std::size_t>::max() - digit) / 10)
            {
                return false;
            }
            length = length * 10 + digit;
        }

        position = colon + 1;
        if (length > encoded.size() - position)
        {
            return false;
        }

        values.emplace_back(encoded.substr(position, length));
        position += length;
    }

    return true;
}

bool parseAssignmentState(
    const std::string& value,
    TimerAssignmentState& state)
{
    if (value == "proposed") state = TimerAssignmentState::proposed;
    else if (value == "selected") state = TimerAssignmentState::selected;
    else if (value == "provisioning")
        state = TimerAssignmentState::provisioning;
    else if (value == "bound") state = TimerAssignmentState::bound;
    else if (value == "reconciling")
        state = TimerAssignmentState::reconciling;
    else if (value == "unassigned")
        state = TimerAssignmentState::unassigned;
    else if (value == "superseding")
        state = TimerAssignmentState::superseding;
    else if (value == "superseded")
        state = TimerAssignmentState::superseded;
    else if (value == "cancel_requested")
        state = TimerAssignmentState::cancelRequested;
    else if (value == "cancelled")
        state = TimerAssignmentState::cancelled;
    else if (value == "failed") state = TimerAssignmentState::failed;
    else return false;
    return true;
}

bool parseAssignmentRole(
    const std::string& value,
    TimerAssignmentRole& role)
{
    if (value == "primary") role = TimerAssignmentRole::primary;
    else if (value == "replica") role = TimerAssignmentRole::replica;
    else if (value == "replacement")
        role = TimerAssignmentRole::replacement;
    else return false;
    return true;
}

bool bindAssignmentColumns(
    sqlite3_stmt* statement,
    int startIndex,
    const TimerAssignment& assignment,
    bool includeId)
{
    int index = startIndex;
    std::int64_t revision = 0;
    if (!parseRevisionToken(
            assignment.assignmentRevision,
            revision))
    {
        return false;
    }

    if (includeId &&
        !bindText(
            statement,
            index++,
            assignment.timerAssignmentId))
    {
        return false;
    }

    if (sqlite3_bind_int64(statement, index++, revision) != SQLITE_OK)
        return false;
    if (!bindText(statement, index++, assignment.timerIntentId))
        return false;
    if (!bindText(statement, index++, assignment.intentRevision))
        return false;
    if (assignment.assignmentEpoch >
        static_cast<std::uint64_t>(
            std::numeric_limits<sqlite3_int64>::max()))
    {
        return false;
    }
    if (sqlite3_bind_int64(
            statement,
            index++,
            static_cast<sqlite3_int64>(
                assignment.assignmentEpoch)) != SQLITE_OK)
    {
        return false;
    }
    if (!bindText(statement, index++, assignment.backendId))
        return false;
    if (assignment.backendGeneration >
        static_cast<std::uint64_t>(
            std::numeric_limits<sqlite3_int64>::max()))
    {
        return false;
    }
    if (sqlite3_bind_int64(
            statement,
            index++,
            static_cast<sqlite3_int64>(
                assignment.backendGeneration)) != SQLITE_OK)
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            timerAssignmentStateName(assignment.state)))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            timerAssignmentRoleName(assignment.role)))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            assignment.channelBinding.canonicalChannelId))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            assignment.channelBinding.backendChannelId))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            assignment.channelBinding.mappingSource))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            assignment.channelBinding.mappingRevision))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            assignment.capabilityRevision))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            assignment.backendHealthRevision))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            assignment.decisionPolicyVersion))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            encodeStrings(assignment.decisionEvidence.reasons)))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            encodeStrings(assignment.decisionEvidence.warnings)))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            encodeStrings(assignment.decisionEvidence.exclusions)))
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            encodeStrings(assignment.decisionEvidence.conflictFacts)))
    {
        return false;
    }
    if (sqlite3_bind_int(
            statement,
            index++,
            assignment.decisionEvidence.decisionScore) != SQLITE_OK)
    {
        return false;
    }
    if (!bindText(
            statement,
            index++,
            assignment.nativeTimerBindingId))
    {
        return false;
    }
    if (sqlite3_bind_int64(
            statement,
            index++,
            assignment.createdAt) != SQLITE_OK)
    {
        return false;
    }
    return sqlite3_bind_int64(
        statement,
        index,
        assignment.updatedAt) == SQLITE_OK;
}

bool readAssignment(
    sqlite3_stmt* statement,
    TimerAssignment& assignment)
{
    const sqlite3_int64 revision =
        sqlite3_column_int64(statement, 1);
    const sqlite3_int64 assignmentEpoch =
        sqlite3_column_int64(statement, 4);
    const sqlite3_int64 backendGeneration =
        sqlite3_column_int64(statement, 6);

    if (revision <= 0 ||
        assignmentEpoch <= 0 ||
        backendGeneration < 0)
    {
        return false;
    }

    assignment.timerAssignmentId = columnText(statement, 0);
    assignment.assignmentRevision = std::to_string(revision);
    assignment.timerIntentId = columnText(statement, 2);
    assignment.intentRevision = columnText(statement, 3);
    assignment.assignmentEpoch =
        static_cast<std::uint64_t>(assignmentEpoch);
    assignment.backendId = columnText(statement, 5);
    assignment.backendGeneration =
        static_cast<std::uint64_t>(backendGeneration);

    if (!parseAssignmentState(
            columnText(statement, 7),
            assignment.state))
    {
        return false;
    }
    if (!parseAssignmentRole(
            columnText(statement, 8),
            assignment.role))
    {
        return false;
    }

    assignment.channelBinding.canonicalChannelId =
        columnText(statement, 9);
    assignment.channelBinding.backendChannelId =
        columnText(statement, 10);
    assignment.channelBinding.mappingSource =
        columnText(statement, 11);
    assignment.channelBinding.mappingRevision =
        columnText(statement, 12);
    assignment.capabilityRevision = columnText(statement, 13);
    assignment.backendHealthRevision = columnText(statement, 14);
    assignment.decisionPolicyVersion = columnText(statement, 15);

    if (!decodeStrings(
            columnText(statement, 16),
            assignment.decisionEvidence.reasons) ||
        !decodeStrings(
            columnText(statement, 17),
            assignment.decisionEvidence.warnings) ||
        !decodeStrings(
            columnText(statement, 18),
            assignment.decisionEvidence.exclusions) ||
        !decodeStrings(
            columnText(statement, 19),
            assignment.decisionEvidence.conflictFacts))
    {
        return false;
    }

    assignment.decisionEvidence.decisionScore =
        sqlite3_column_int(statement, 20);
    assignment.nativeTimerBindingId = columnText(statement, 21);
    assignment.createdAt = sqlite3_column_int64(statement, 22);
    assignment.updatedAt = sqlite3_column_int64(statement, 23);

    return timerAssignmentValid(assignment);
}

bool selectById(
    Database& database,
    const std::string& timerAssignmentId,
    TimerAssignment& assignment,
    bool& found)
{
    found = false;
    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string("SELECT ") + kSelectColumns +
        " FROM timer_assignments "
        "WHERE timer_assignment_id=?;";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    if (!bindText(statement, 1, timerAssignmentId))
    {
        sqlite3_finalize(statement);
        return false;
    }

    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        found = readAssignment(statement, assignment);
        const bool ok = found;
        sqlite3_finalize(statement);
        return ok;
    }

    const bool ok = step == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

enum class IntentRevisionLookup
{
    found,
    notFound,
    storageError,
};

IntentRevisionLookup findIntentRevision(
    Database& database,
    const std::string& timerIntentId,
    std::string& revision)
{
    revision.clear();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT intent_revision FROM timer_intents "
        "WHERE timer_intent_id=?;";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return IntentRevisionLookup::storageError;
    }

    if (!bindText(statement, 1, timerIntentId))
    {
        sqlite3_finalize(statement);
        return IntentRevisionLookup::storageError;
    }

    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        revision = columnText(statement, 0);
        sqlite3_finalize(statement);
        return revision.empty()
            ? IntentRevisionLookup::storageError
            : IntentRevisionLookup::found;
    }

    sqlite3_finalize(statement);
    return step == SQLITE_DONE
        ? IntentRevisionLookup::notFound
        : IntentRevisionLookup::storageError;
}

bool nextAssignmentEpoch(
    Database& database,
    const std::string& timerIntentId,
    std::uint64_t& epoch)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT COALESCE(MAX(assignment_epoch),0) "
        "FROM timer_assignments "
        "WHERE timer_intent_id=?;";

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

    const sqlite3_int64 current =
        sqlite3_column_int64(statement, 0);
    sqlite3_finalize(statement);

    if (current < 0 ||
        current == std::numeric_limits<sqlite3_int64>::max())
    {
        return false;
    }

    epoch = static_cast<std::uint64_t>(current) + 1;
    return epoch > 0;
}

bool hasOtherActivePrimary(
    Database& database,
    const std::string& timerIntentId,
    const std::string& excludedAssignmentId,
    bool& present)
{
    present = false;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT timer_assignment_id "
        "FROM timer_assignments "
        "WHERE timer_intent_id=? "
        "AND role IN ('primary','replacement') "
        "AND state IN "
        "('selected','provisioning','bound',"
        "'reconciling','superseding') "
        "AND timer_assignment_id<>? "
        "LIMIT 1;";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    if (!bindText(statement, 1, timerIntentId) ||
        !bindText(statement, 2, excludedAssignmentId))
    {
        sqlite3_finalize(statement);
        return false;
    }

    const int step = sqlite3_step(statement);
    present = step == SQLITE_ROW;
    const bool ok = present || step == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

TimerAssignmentRepositoryResult statusResult(
    TimerAssignmentRepositoryStatus status)
{
    TimerAssignmentRepositoryResult result;
    result.status = status;
    return result;
}

TimerAssignmentRepositoryResult statusResult(
    TimerAssignmentRepositoryStatus status,
    const TimerAssignment& assignment)
{
    TimerAssignmentRepositoryResult result;
    result.status = status;
    result.assignment = assignment;
    return result;
}

TimerAssignmentRepositoryStatus validateIntentRevision(
    Database& database,
    const TimerAssignment& assignment)
{
    if (!safeIdentity(assignment.timerIntentId) ||
        assignment.intentRevision.empty())
    {
        return TimerAssignmentRepositoryStatus::invalid;
    }

    std::string currentRevision;
    const IntentRevisionLookup lookup =
        findIntentRevision(
            database,
            assignment.timerIntentId,
            currentRevision);

    if (lookup == IntentRevisionLookup::notFound)
    {
        return TimerAssignmentRepositoryStatus::intentNotFound;
    }
    if (lookup == IntentRevisionLookup::storageError)
    {
        return TimerAssignmentRepositoryStatus::storageError;
    }
    if (currentRevision != assignment.intentRevision)
    {
        return TimerAssignmentRepositoryStatus::intentRevisionConflict;
    }
    return TimerAssignmentRepositoryStatus::ok;
}

}

TimerAssignmentRepository::TimerAssignmentRepository(
    Database& database)
    : database_(database)
{
}

bool TimerAssignmentRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();

    return database_.execute(
        "CREATE TABLE IF NOT EXISTS timer_assignments ("
        "timer_assignment_id TEXT PRIMARY KEY NOT NULL,"
        "assignment_revision INTEGER NOT NULL "
        "CHECK(assignment_revision > 0),"
        "timer_intent_id TEXT NOT NULL,"
        "intent_revision TEXT NOT NULL,"
        "assignment_epoch INTEGER NOT NULL "
        "CHECK(assignment_epoch > 0),"
        "backend_id TEXT NOT NULL DEFAULT '',"
        "backend_generation INTEGER NOT NULL DEFAULT 0 "
        "CHECK(backend_generation >= 0),"
        "state TEXT NOT NULL CHECK(state IN "
        "('proposed','selected','provisioning','bound',"
        "'reconciling','unassigned','superseding','superseded',"
        "'cancel_requested','cancelled','failed')),"
        "role TEXT NOT NULL CHECK(role IN "
        "('primary','replica','replacement')),"
        "channel_canonical_id TEXT NOT NULL DEFAULT '',"
        "channel_backend_id TEXT NOT NULL DEFAULT '',"
        "channel_mapping_source TEXT NOT NULL DEFAULT '',"
        "channel_mapping_revision TEXT NOT NULL DEFAULT '',"
        "capability_revision TEXT NOT NULL DEFAULT '',"
        "backend_health_revision TEXT NOT NULL DEFAULT '',"
        "decision_policy_version TEXT NOT NULL,"
        "decision_reasons TEXT NOT NULL,"
        "decision_warnings TEXT NOT NULL DEFAULT '',"
        "decision_exclusions TEXT NOT NULL DEFAULT '',"
        "decision_conflict_facts TEXT NOT NULL DEFAULT '',"
        "decision_score INTEGER NOT NULL DEFAULT 0,"
        "native_timer_binding_id TEXT NOT NULL DEFAULT '',"
        "created_at INTEGER NOT NULL,"
        "updated_at INTEGER NOT NULL,"
        "UNIQUE(timer_intent_id,assignment_epoch),"
        "FOREIGN KEY(timer_intent_id) "
        "REFERENCES timer_intents(timer_intent_id)"
        ");"
        "CREATE INDEX IF NOT EXISTS "
        "idx_timer_assignments_intent_epoch "
        "ON timer_assignments "
        "(timer_intent_id,assignment_epoch,timer_assignment_id);"
        "CREATE UNIQUE INDEX IF NOT EXISTS "
        "idx_timer_assignments_active_exclusive_owner "
        "ON timer_assignments(timer_intent_id) "
        "WHERE role IN ('primary','replacement') AND state IN "
        "('selected','provisioning','bound',"
        "'reconciling','superseding');"
        "DROP INDEX IF EXISTS idx_timer_assignments_active_primary;");
}

TimerAssignmentRepositoryResult
TimerAssignmentRepository::create(
    const TimerAssignment& assignment)
{
    if (!assignment.assignmentRevision.empty() ||
        assignment.assignmentEpoch != 0 ||
        !safeIdentity(assignment.timerAssignmentId) ||
        !safeIdentity(assignment.timerIntentId))
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::invalid);
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema() ||
        !database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    TimerAssignment existing;
    bool found = false;
    if (!selectById(
            database_,
            assignment.timerAssignmentId,
            existing,
            found))
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }
    if (found)
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::alreadyExists);
    }

    const TimerAssignmentRepositoryStatus intentStatus =
        validateIntentRevision(database_, assignment);
    if (intentStatus != TimerAssignmentRepositoryStatus::ok)
    {
        database_.execute("ROLLBACK;");
        return statusResult(intentStatus);
    }

    TimerAssignment durable = assignment;
    durable.assignmentRevision = "1";
    durable.updatedAt = durable.createdAt;
    if (!nextAssignmentEpoch(
            database_,
            durable.timerIntentId,
            durable.assignmentEpoch) ||
        !timerAssignmentValid(durable))
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::invalid);
    }

    if ((durable.role == TimerAssignmentRole::primary
         || durable.role == TimerAssignmentRole::replacement) &&
        timerAssignmentActiveOwnershipState(durable.state))
    {
        if (durable.role == TimerAssignmentRole::replacement
            && !controlledReplacementExpected(
                database_, durable.timerAssignmentId))
        {
            database_.execute("ROLLBACK;");
            return statusResult(
                TimerAssignmentRepositoryStatus::ownershipConflict);
        }
        bool activePrimary = false;
        if (durable.role != TimerAssignmentRole::replacement
            && !hasOtherActivePrimary(
                database_,
                durable.timerIntentId,
                "",
                activePrimary))
        {
            database_.execute("ROLLBACK;");
            return statusResult(
                TimerAssignmentRepositoryStatus::storageError);
        }
        if (durable.role != TimerAssignmentRole::replacement
            && activePrimary)
        {
            database_.execute("ROLLBACK;");
            return statusResult(
                TimerAssignmentRepositoryStatus::ownershipConflict);
        }
    }

    const char* sql =
        "INSERT INTO timer_assignments ("
        "timer_assignment_id,assignment_revision,timer_intent_id,"
        "intent_revision,assignment_epoch,backend_id,"
        "backend_generation,state,role,channel_canonical_id,"
        "channel_backend_id,channel_mapping_source,"
        "channel_mapping_revision,capability_revision,"
        "backend_health_revision,decision_policy_version,"
        "decision_reasons,decision_warnings,decision_exclusions,"
        "decision_conflict_facts,decision_score,"
        "native_timer_binding_id,created_at,updated_at"
        ") VALUES "
        "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    if (!bindAssignmentColumns(
            statement,
            1,
            durable,
            true))
    {
        sqlite3_finalize(statement);
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    const int step = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (step != SQLITE_DONE)
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            sqlite3_errcode(database_.handle()) == SQLITE_CONSTRAINT
                ? TimerAssignmentRepositoryStatus::ownershipConflict
                : TimerAssignmentRepositoryStatus::storageError);
    }

    if (!database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    return statusResult(
        TimerAssignmentRepositoryStatus::ok,
        durable);
}

TimerAssignmentRepositoryResult
TimerAssignmentRepository::findById(
    const std::string& timerAssignmentId)
{
    if (!safeIdentity(timerAssignmentId))
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::invalid);
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema())
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    TimerAssignment assignment;
    bool found = false;
    if (!selectById(
            database_,
            timerAssignmentId,
            assignment,
            found))
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }
    if (!found)
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::notFound);
    }

    return statusResult(
        TimerAssignmentRepositoryStatus::ok,
        assignment);
}

TimerAssignmentRepositoryListResult
TimerAssignmentRepository::listForIntent(
    const std::string& timerIntentId)
{
    TimerAssignmentRepositoryListResult result;
    if (!safeIdentity(timerIntentId))
    {
        result.status = TimerAssignmentRepositoryStatus::invalid;
        return result;
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema())
    {
        result.status =
            TimerAssignmentRepositoryStatus::storageError;
        return result;
    }

    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string("SELECT ") + kSelectColumns +
        " FROM timer_assignments "
        "WHERE timer_intent_id=? "
        "ORDER BY assignment_epoch,timer_assignment_id;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        result.status =
            TimerAssignmentRepositoryStatus::storageError;
        return result;
    }

    if (!bindText(statement, 1, timerIntentId))
    {
        sqlite3_finalize(statement);
        result.status =
            TimerAssignmentRepositoryStatus::storageError;
        return result;
    }

    int step = SQLITE_ROW;
    while ((step = sqlite3_step(statement)) == SQLITE_ROW)
    {
        TimerAssignment assignment;
        if (!readAssignment(statement, assignment))
        {
            sqlite3_finalize(statement);
            result.assignments.clear();
            result.status =
                TimerAssignmentRepositoryStatus::storageError;
            return result;
        }
        result.assignments.push_back(assignment);
    }

    sqlite3_finalize(statement);
    result.status = step == SQLITE_DONE
        ? TimerAssignmentRepositoryStatus::ok
        : TimerAssignmentRepositoryStatus::storageError;
    if (!result.ok())
    {
        result.assignments.clear();
    }
    return result;
}

TimerAssignmentRepositoryResult
TimerAssignmentRepository::findActivePrimaryForIntent(
    const std::string& timerIntentId)
{
    if (!safeIdentity(timerIntentId))
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::invalid);
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema())
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string("SELECT ") + kSelectColumns +
        " FROM timer_assignments "
        "WHERE timer_intent_id=? "
        "AND role IN ('primary','replacement') "
        "AND state IN "
        "('selected','provisioning','bound',"
        "'reconciling','superseding') "
        "ORDER BY assignment_epoch,timer_assignment_id "
        "LIMIT 2;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    if (!bindText(statement, 1, timerIntentId))
    {
        sqlite3_finalize(statement);
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    TimerAssignment assignment;
    const int first = sqlite3_step(statement);
    if (first == SQLITE_DONE)
    {
        sqlite3_finalize(statement);
        return statusResult(
            TimerAssignmentRepositoryStatus::notFound);
    }
    if (first != SQLITE_ROW ||
        !readAssignment(statement, assignment))
    {
        sqlite3_finalize(statement);
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    const int second = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (second != SQLITE_DONE)
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    return statusResult(
        TimerAssignmentRepositoryStatus::ok,
        assignment);
}

TimerAssignmentRepositoryResult
TimerAssignmentRepository::update(
    const TimerAssignment& next,
    const std::string& expectedRevision)
{
    std::int64_t expectedRevisionNumber = 0;
    if (!parseRevisionToken(
            expectedRevision,
            expectedRevisionNumber) ||
        next.assignmentRevision != expectedRevision ||
        !safeIdentity(next.timerAssignmentId))
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::invalid);
    }

    auto lease = database_.acquireTransactionLease();
    if (!ensureSchema() ||
        !database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    TimerAssignment current;
    bool found = false;
    if (!selectById(
            database_,
            next.timerAssignmentId,
            current,
            found))
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }
    if (!found)
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::notFound);
    }
    if (!timerAssignmentRevisionMatches(
            expectedRevision,
            current.assignmentRevision))
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::conflict,
            current);
    }

    if (timerAssignmentTerminal(current.state) ||
        next.timerIntentId != current.timerIntentId ||
        next.assignmentEpoch != current.assignmentEpoch ||
        next.role != current.role ||
        next.createdAt != current.createdAt ||
        next.updatedAt <= current.updatedAt ||
        (!current.backendId.empty() &&
         next.backendId != current.backendId) ||
        (next.state != current.state &&
         !timerAssignmentCanTransition(
             current.state,
             next.state)))
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::invalid);
    }

    const TimerAssignmentRepositoryStatus intentStatus =
        validateIntentRevision(database_, next);
    if (intentStatus != TimerAssignmentRepositoryStatus::ok)
    {
        database_.execute("ROLLBACK;");
        return statusResult(intentStatus);
    }

    TimerAssignment durable = next;
    durable.assignmentRevision =
        std::to_string(expectedRevisionNumber + 1);
    if (!timerAssignmentValid(durable))
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::invalid);
    }

    if ((durable.role == TimerAssignmentRole::primary
         || durable.role == TimerAssignmentRole::replacement) &&
        timerAssignmentActiveOwnershipState(durable.state))
    {
        bool activePrimary = false;
        if (!hasOtherActivePrimary(
                database_,
                durable.timerIntentId,
                durable.timerAssignmentId,
                activePrimary))
        {
            database_.execute("ROLLBACK;");
            return statusResult(
                TimerAssignmentRepositoryStatus::storageError);
        }
        if (activePrimary)
        {
            database_.execute("ROLLBACK;");
            return statusResult(
                TimerAssignmentRepositoryStatus::ownershipConflict);
        }
    }

    const char* sql =
        "UPDATE timer_assignments SET "
        "assignment_revision=?,timer_intent_id=?,intent_revision=?,"
        "assignment_epoch=?,backend_id=?,backend_generation=?,"
        "state=?,role=?,channel_canonical_id=?,channel_backend_id=?,"
        "channel_mapping_source=?,channel_mapping_revision=?,"
        "capability_revision=?,backend_health_revision=?,"
        "decision_policy_version=?,decision_reasons=?,"
        "decision_warnings=?,decision_exclusions=?,"
        "decision_conflict_facts=?,decision_score=?,"
        "native_timer_binding_id=?,created_at=?,updated_at=? "
        "WHERE timer_assignment_id=? "
        "AND assignment_revision=?;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    if (!bindAssignmentColumns(
            statement,
            1,
            durable,
            false) ||
        !bindText(
            statement,
            24,
            durable.timerAssignmentId) ||
        sqlite3_bind_int64(
            statement,
            25,
            expectedRevisionNumber) != SQLITE_OK)
    {
        sqlite3_finalize(statement);
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    const int step = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (step != SQLITE_DONE)
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            sqlite3_errcode(database_.handle()) == SQLITE_CONSTRAINT
                ? TimerAssignmentRepositoryStatus::ownershipConflict
                : TimerAssignmentRepositoryStatus::storageError);
    }
    if (sqlite3_changes(database_.handle()) != 1)
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    if (!database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return statusResult(
            TimerAssignmentRepositoryStatus::storageError);
    }

    return statusResult(
        TimerAssignmentRepositoryStatus::ok,
        durable);
}

}
