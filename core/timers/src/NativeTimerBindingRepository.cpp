#include "NativeTimerBindingRepository.h"

#include "Database.h"
#include "NativeTimerBindingRepositoryStorage.h"

#include <cstdint>
#include <sqlite3.h>
#include <vector>
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

bool representableGeneration(std::uint64_t value)
{
    return value <= static_cast<std::uint64_t>(
        std::numeric_limits<std::int64_t>::max());
}

bool parseRevision(const std::string& token, std::int64_t& value)
{
    if (token.empty()) return false;
    value = 0;
    for (char ch : token)
    {
        if (ch < '0' || ch > '9') return false;
        const std::int64_t digit = ch - '0';
        if (value > (std::numeric_limits<std::int64_t>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    return value > 0;
}

bool managedOwnership(NativeTimerBindingOwnership ownership)
{
    return ownership == NativeTimerBindingOwnership::managed ||
        ownership == NativeTimerBindingOwnership::adopted;
}

// The passive same-Timer proof must move the bound Assignment's generation
// with the Binding in the SAME SQLite transaction. Otherwise public GET works
// after an Agent restart while UPDATE/TOGGLE/DELETE fail with HTTP 409.
// Only this exact bound identity and its current active/paused intent qualify.
bool refreshBoundAssignmentGeneration(
    Database& database, const NativeTimerBinding& binding,
    std::uint64_t generation, std::int64_t observedAt)
{
    sqlite3_stmt* read = nullptr;
    const char* sql =
        "SELECT a.backend_generation,a.assignment_revision,a.updated_at "
        "FROM timer_assignments a JOIN timer_intents i "
        "ON i.timer_intent_id=a.timer_intent_id "
        "AND CAST(a.intent_revision AS INTEGER)=i.intent_revision "
        "WHERE a.timer_assignment_id=? AND a.native_timer_binding_id=? "
        "AND a.backend_id=? AND a.state='bound' "
        "AND i.state IN ('active','paused');";
    if (sqlite3_prepare_v2(database.handle(), sql, -1, &read, nullptr)
            != SQLITE_OK)
        return false;
    const bool bound =
        sqlite3_bind_text(read, 1, binding.timerAssignmentId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_text(read, 2, binding.nativeTimerBindingId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_text(read, 3, binding.backendId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK;
    const int step = bound ? sqlite3_step(read) : SQLITE_ERROR;
    const std::int64_t oldGeneration =
        step == SQLITE_ROW ? sqlite3_column_int64(read, 0) : -1;
    const std::int64_t oldRevision =
        step == SQLITE_ROW ? sqlite3_column_int64(read, 1) : -1;
    const std::int64_t oldTime =
        step == SQLITE_ROW ? sqlite3_column_int64(read, 2) : -1;
    sqlite3_finalize(read);
    if (step != SQLITE_ROW || oldGeneration <= 0 ||
        oldGeneration > static_cast<std::int64_t>(generation) ||
        oldRevision <= 0 ||
        oldRevision == std::numeric_limits<std::int64_t>::max() ||
        oldTime <= 0 || oldTime == std::numeric_limits<std::int64_t>::max())
        return false;
    if (oldGeneration == static_cast<std::int64_t>(generation))
        return true;

    sqlite3_stmt* update = nullptr;
    const char* writeSql =
        "UPDATE timer_assignments SET backend_generation=?, "
        "assignment_revision=assignment_revision+1, "
        "updated_at=MAX(updated_at+1,?) "
        "WHERE timer_assignment_id=? AND native_timer_binding_id=? "
        "AND backend_id=? AND state='bound' AND backend_generation=? "
        "AND assignment_revision=?;";
    if (sqlite3_prepare_v2(database.handle(), writeSql, -1, &update, nullptr)
            != SQLITE_OK)
        return false;
    const bool ready =
        sqlite3_bind_int64(update, 1, static_cast<sqlite3_int64>(generation))
            == SQLITE_OK &&
        sqlite3_bind_int64(update, 2, observedAt) == SQLITE_OK &&
        sqlite3_bind_text(update, 3, binding.timerAssignmentId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_text(update, 4, binding.nativeTimerBindingId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_text(update, 5, binding.backendId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_int64(update, 6, oldGeneration) == SQLITE_OK &&
        sqlite3_bind_int64(update, 7, oldRevision) == SQLITE_OK;
    const int written = ready ? sqlite3_step(update) : SQLITE_ERROR;
    sqlite3_finalize(update);
    return written == SQLITE_DONE && sqlite3_changes(database.handle()) == 1;
}

NativeTimerBindingRepositoryResult statusResult(
    NativeTimerBindingRepositoryStatus status)
{
    NativeTimerBindingRepositoryResult result;
    result.status = status;
    return result;
}

NativeTimerBindingRepositoryResult bindingResult(
    NativeTimerBindingRepositoryStatus status,
    const NativeTimerBinding& binding)
{
    NativeTimerBindingRepositoryResult result;
    result.status = status;
    result.binding = binding;
    return result;
}
}

NativeTimerBindingRepository::NativeTimerBindingRepository(Database& database)
    : database_(database)
{
}

bool NativeTimerBindingRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS native_timer_bindings ("
        "native_timer_binding_id TEXT PRIMARY KEY,"
        "binding_revision INTEGER NOT NULL,backend_id TEXT NOT NULL,"
        "backend_generation INTEGER NOT NULL,backend_native_timer_id TEXT NOT NULL,"
        "timer_assignment_id TEXT NOT NULL,ownership TEXT NOT NULL,"
        "observed_fingerprint TEXT NOT NULL,observed_channel_id TEXT NOT NULL,"
        "observed_event_id TEXT NOT NULL,observed_title TEXT NOT NULL,"
        "observed_directory TEXT NOT NULL,observed_day TEXT NOT NULL,"
        "observed_weekdays TEXT NOT NULL,observed_start_time TEXT NOT NULL,"
        "observed_end_time TEXT NOT NULL,observed_flags INTEGER NOT NULL,"
        "observed_priority INTEGER NOT NULL,observed_lifetime INTEGER NOT NULL,"
        "observed_enabled INTEGER NOT NULL,observed_vps INTEGER NOT NULL,"
        "observed_recording INTEGER NOT NULL,observed_pending INTEGER NOT NULL,"
        "last_observed_at INTEGER NOT NULL,last_verified_operation_id TEXT NOT NULL,"
        "missing_since INTEGER NOT NULL,drift_state TEXT NOT NULL);"
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_native_timer_bindings_native_identity "
        "ON native_timer_bindings(backend_id,backend_native_timer_id);"
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_native_timer_bindings_managed_assignment "
        "ON native_timer_bindings(timer_assignment_id) "
        "WHERE timer_assignment_id<>'' AND ownership IN ('managed','adopted');"
        "CREATE INDEX IF NOT EXISTS idx_native_timer_bindings_assignment "
        "ON native_timer_bindings(timer_assignment_id);");
}

NativeTimerBindingRepositoryResult NativeTimerBindingRepository::create(
    const NativeTimerBinding& binding)
{
    if (!safeIdentity(binding.nativeTimerBindingId) ||
        !binding.bindingRevision.empty() ||
        !representableGeneration(binding.backendGeneration))
    {
        return statusResult(NativeTimerBindingRepositoryStatus::invalid);
    }

    NativeTimerBinding durable = binding;
    durable.bindingRevision = "1";
    if (!nativeTimerBindingValid(durable))
        return statusResult(NativeTimerBindingRepositoryStatus::invalid);

    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    auto rollback = [this]() { database_.execute("ROLLBACK;"); };

    NativeTimerBinding existing;
    bool found = false;
    if (!detail::nativeBindingSelectById(
            database_, durable.nativeTimerBindingId, existing, found))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    if (found)
    {
        rollback();
        return bindingResult(
            NativeTimerBindingRepositoryStatus::alreadyExists, existing);
    }

    if (!detail::nativeBindingSelectByNativeIdentity(
            database_, durable.backendId, durable.backendNativeTimerId,
            existing, found))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    if (found)
    {
        rollback();
        return bindingResult(
            NativeTimerBindingRepositoryStatus::nativeIdentityConflict, existing);
    }

    if (managedOwnership(durable.ownership))
    {
        if (!detail::nativeBindingSelectManagedForAssignment(
                database_, durable.timerAssignmentId, existing, found))
        {
            rollback();
            return statusResult(NativeTimerBindingRepositoryStatus::storageError);
        }
        if (found)
        {
            rollback();
            return bindingResult(
                NativeTimerBindingRepositoryStatus::assignmentBindingConflict,
                existing);
        }
    }

    if (!detail::nativeBindingInsert(database_, durable))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    if (!database_.execute("COMMIT;"))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    return bindingResult(NativeTimerBindingRepositoryStatus::ok, durable);
}

NativeTimerBindingRepositoryResult NativeTimerBindingRepository::findById(
    const std::string& nativeTimerBindingId)
{
    if (!safeIdentity(nativeTimerBindingId))
        return statusResult(NativeTimerBindingRepositoryStatus::invalid);
    NativeTimerBinding binding;
    bool found = false;
    if (!detail::nativeBindingSelectById(
            database_, nativeTimerBindingId, binding, found))
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    return found
        ? bindingResult(NativeTimerBindingRepositoryStatus::ok, binding)
        : statusResult(NativeTimerBindingRepositoryStatus::notFound);
}

NativeTimerBindingRepositoryResult
NativeTimerBindingRepository::findByBackendNativeTimer(
    const std::string& backendId,
    const std::string& backendNativeTimerId)
{
    if (!safeIdentity(backendId) || !safeIdentity(backendNativeTimerId))
        return statusResult(NativeTimerBindingRepositoryStatus::invalid);
    NativeTimerBinding binding;
    bool found = false;
    if (!detail::nativeBindingSelectByNativeIdentity(
            database_, backendId, backendNativeTimerId, binding, found))
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    return found
        ? bindingResult(NativeTimerBindingRepositoryStatus::ok, binding)
        : statusResult(NativeTimerBindingRepositoryStatus::notFound);
}

NativeTimerBindingRepositoryListResult
NativeTimerBindingRepository::listForAssignment(
    const std::string& timerAssignmentId)
{
    NativeTimerBindingRepositoryListResult result;
    if (!safeIdentity(timerAssignmentId))
    {
        result.status = NativeTimerBindingRepositoryStatus::invalid;
        return result;
    }
    if (!detail::nativeBindingListForAssignment(
            database_, timerAssignmentId, result.bindings))
    {
        result.status = NativeTimerBindingRepositoryStatus::storageError;
        return result;
    }
    result.status = NativeTimerBindingRepositoryStatus::ok;
    return result;
}

NativeTimerBindingRepositoryListResult
NativeTimerBindingRepository::listManagedForFreshness(
    const std::string& afterId, std::int64_t staleBefore, std::size_t limit)
{
    NativeTimerBindingRepositoryListResult result;
    if (afterId.size() > kMaxIdentityLength || staleBefore <= 0 ||
        limit == 0 || limit > 8)
    {
        result.status = NativeTimerBindingRepositoryStatus::invalid;
        return result;
    }

    // Query IDs first, then hydrate through the existing authoritative
    // repository parser. A keyset cursor prevents one drifted binding from
    // starving other managed timers on every polling tick.
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT native_timer_binding_id FROM native_timer_bindings "
        "WHERE native_timer_binding_id > ? "
        "AND ownership IN ('managed','adopted') AND missing_since=0 "
        "AND drift_state='none' AND observed_recording=0 "
        "AND observed_pending=0 AND last_observed_at < ? "
        "ORDER BY native_timer_binding_id ASC LIMIT ?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr)
            != SQLITE_OK)
    {
        if (statement) sqlite3_finalize(statement);
        result.status = NativeTimerBindingRepositoryStatus::storageError;
        return result;
    }
    const bool bound =
        sqlite3_bind_text(statement, 1, afterId.c_str(), -1, SQLITE_TRANSIENT)
            == SQLITE_OK &&
        sqlite3_bind_int64(statement, 2, staleBefore) == SQLITE_OK &&
        sqlite3_bind_int(statement, 3, static_cast<int>(limit)) == SQLITE_OK;
    std::vector<std::string> ids;
    int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    while (step == SQLITE_ROW)
    {
        const unsigned char* value = sqlite3_column_text(statement, 0);
        if (!value)
        {
            step = SQLITE_ERROR;
            break;
        }
        ids.emplace_back(reinterpret_cast<const char*>(value));
        step = sqlite3_step(statement);
    }
    sqlite3_finalize(statement);
    if (step != SQLITE_DONE)
    {
        result.status = NativeTimerBindingRepositoryStatus::storageError;
        return result;
    }
    for (const auto& id : ids)
    {
        const auto found = findById(id);
        if (!found.ok())
        {
            result.bindings.clear();
            result.status = NativeTimerBindingRepositoryStatus::storageError;
            return result;
        }
        result.bindings.push_back(found.binding);
    }
    result.status = NativeTimerBindingRepositoryStatus::ok;
    return result;
}

NativeTimerBindingRepositoryResult NativeTimerBindingRepository::update(
    const NativeTimerBinding& next,
    const std::string& expectedRevision,
    bool passiveFreshnessRefresh)
{
    std::int64_t expected = 0;
    if (!safeIdentity(next.nativeTimerBindingId) ||
        !parseRevision(expectedRevision, expected) ||
        next.bindingRevision != expectedRevision ||
        !representableGeneration(next.backendGeneration) ||
        !nativeTimerBindingValid(next))
    {
        return statusResult(NativeTimerBindingRepositoryStatus::invalid);
    }

    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    auto rollback = [this]() { database_.execute("ROLLBACK;"); };

    NativeTimerBinding current;
    bool found = false;
    if (!detail::nativeBindingSelectById(
            database_, next.nativeTimerBindingId, current, found))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    if (!found)
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::notFound);
    }
    if (!nativeTimerBindingRevisionMatches(
            expectedRevision, current.bindingRevision))
    {
        rollback();
        return bindingResult(
            NativeTimerBindingRepositoryStatus::conflict, current);
    }

    if (next.backendId != current.backendId ||
        next.backendNativeTimerId != current.backendNativeTimerId ||
        next.timerAssignmentId != current.timerAssignmentId ||
        next.ownership != current.ownership)
    {
        rollback();
        return bindingResult(
            NativeTimerBindingRepositoryStatus::immutableConflict, current);
    }
    if (next.backendGeneration < current.backendGeneration)
    {
        rollback();
        return bindingResult(
            NativeTimerBindingRepositoryStatus::generationConflict, current);
    }
    if (next.lastObservedAt < current.lastObservedAt)
    {
        rollback();
        return bindingResult(
            NativeTimerBindingRepositoryStatus::observationConflict, current);
    }
    if (passiveFreshnessRefresh)
    {
        // An unchanged native fingerprint with a newer authoritative read may
        // advance the Agent generation. Never adopt, reconcile drift,
        // or rewrite in-flight mutation evidence.
        if (!managedOwnership(current.ownership) ||
            current.missingSince != 0 ||
            current.driftState != NativeTimerBindingDriftState::none ||
            current.observedState.recording || current.observedState.pending ||
            current.backendGeneration > next.backendGeneration ||
            current.observedFingerprint != next.observedFingerprint ||
            current.lastVerifiedOperationId != next.lastVerifiedOperationId ||
            current.missingSince != next.missingSince ||
            current.driftState != next.driftState ||
            current.lastObservedAt >= next.lastObservedAt)
        {
            rollback();
            return bindingResult(
                NativeTimerBindingRepositoryStatus::conflict, current);
        }

        // This check and the revision update share BEGIN IMMEDIATE. A new
        // timer mutation cannot be reserved between them (including through
        // another SQLite connection), so accepted/uncertain work retains its
        // binding-revision fence.
        sqlite3_stmt* pending = nullptr;
        const char* sql =
            "SELECT 1 FROM mutation_operations "
            "WHERE ((resource_type='NativeTimerBinding' AND resource_id=?) "
            "OR (resource_type='TimerAssignment' AND resource_id=?)) "
            "AND state NOT IN ('rejected','conflict','succeeded',"
            "'failed_before_dispatch','failed_verified','cancelled') LIMIT 1;";
        if (sqlite3_prepare_v2(database_.handle(), sql, -1, &pending, nullptr)
                != SQLITE_OK)
        {
            if (pending) sqlite3_finalize(pending);
            rollback();
            return statusResult(NativeTimerBindingRepositoryStatus::storageError);
        }
        const bool bound =
            sqlite3_bind_text(pending, 1,
                current.nativeTimerBindingId.c_str(), -1,
                SQLITE_TRANSIENT) == SQLITE_OK &&
            sqlite3_bind_text(pending, 2,
                current.timerAssignmentId.c_str(), -1,
                SQLITE_TRANSIENT) == SQLITE_OK;
        const int step = bound ? sqlite3_step(pending) : SQLITE_ERROR;
        sqlite3_finalize(pending);
        if (step != SQLITE_DONE)
        {
            rollback();
            return step == SQLITE_ROW
                ? bindingResult(NativeTimerBindingRepositoryStatus::conflict, current)
                : statusResult(NativeTimerBindingRepositoryStatus::storageError);
        }
    }

    if (expected == std::numeric_limits<std::int64_t>::max())
    {
        rollback();
        return bindingResult(
            NativeTimerBindingRepositoryStatus::conflict, current);
    }

    NativeTimerBinding durable = next;
    durable.bindingRevision = std::to_string(expected + 1);
    if (!nativeTimerBindingValid(durable) ||
        !detail::nativeBindingUpdate(database_, durable, expected))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    if (passiveFreshnessRefresh &&
        !refreshBoundAssignmentGeneration(database_, current, durable.backendGeneration,
            durable.lastObservedAt))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::conflict);
    }
    if (!database_.execute("COMMIT;"))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    return bindingResult(NativeTimerBindingRepositoryStatus::ok, durable);
}


NativeTimerBindingRepositoryResult
NativeTimerBindingRepository::updateVerifiedNativeIdentity(
    const NativeTimerBinding& next, const std::string& expectedRevision)
{
    std::int64_t revision = 0;
    if (!parseRevision(expectedRevision, revision) ||
        revision == std::numeric_limits<std::int64_t>::max() ||
        next.bindingRevision != expectedRevision ||
        !nativeTimerBindingValid(next) ||
        !safeIdentity(next.backendNativeTimerId) ||
        next.lastVerifiedOperationId.empty() ||
        !representableGeneration(next.backendGeneration))
        return statusResult(NativeTimerBindingRepositoryStatus::invalid);

    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    auto rollback = [this]() { database_.execute("ROLLBACK;"); };
    NativeTimerBinding current;
    bool found = false;
    if (!detail::nativeBindingSelectById(
            database_, next.nativeTimerBindingId, current, found))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    if (!found || current.bindingRevision != expectedRevision)
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::conflict);
    }
    if (next.backendNativeTimerId == current.backendNativeTimerId ||
        next.backendId != current.backendId ||
        next.timerAssignmentId != current.timerAssignmentId ||
        next.ownership != current.ownership ||
        !managedOwnership(current.ownership) ||
        current.missingSince != 0 ||
        current.driftState != NativeTimerBindingDriftState::none ||
        current.backendGeneration != next.backendGeneration ||
        next.lastObservedAt < current.lastObservedAt ||
        next.observedFingerprint == current.observedFingerprint ||
        next.lastVerifiedOperationId == current.lastVerifiedOperationId)
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::immutableConflict);
    }

    // This is NOT a generic rename. Its authority is a durable UPDATE
    // whose exact predecessor binding/generation is still current.
    sqlite3_stmt* guard = nullptr;
    const char* check =
        "SELECT 1 FROM mutation_operations WHERE operation_id=? "
        "AND action_family='timer.update' "
        "AND resource_type='NativeTimerBinding' AND resource_id=? "
        "AND backend_id=? AND backend_generation=? "
        "AND expected_revision=? "
        "AND state IN ('executed_unverified','outcome_unknown') LIMIT 1;";
    if (sqlite3_prepare_v2(database_.handle(), check, -1, &guard, nullptr)
            != SQLITE_OK)
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    const bool ok =
        sqlite3_bind_text(guard, 1, next.lastVerifiedOperationId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_text(guard, 2, current.nativeTimerBindingId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_text(guard, 3, current.backendId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_int64(guard, 4,
            static_cast<sqlite3_int64>(current.backendGeneration)) == SQLITE_OK &&
        sqlite3_bind_text(guard, 5, expectedRevision.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK;
    const int guardStep = ok ? sqlite3_step(guard) : SQLITE_ERROR;
    sqlite3_finalize(guard);
    if (guardStep != SQLITE_ROW)
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::conflict);
    }

    sqlite3_stmt* rename = nullptr;
    const char* sql =
        "UPDATE native_timer_bindings SET backend_native_timer_id=? "
        "WHERE native_timer_binding_id=? AND binding_revision=? "
        "AND backend_native_timer_id=?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &rename, nullptr)
            != SQLITE_OK)
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    const bool bound =
        sqlite3_bind_text(rename, 1, next.backendNativeTimerId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_text(rename, 2, current.nativeTimerBindingId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_int64(rename, 3, revision) == SQLITE_OK &&
        sqlite3_bind_text(rename, 4, current.backendNativeTimerId.c_str(), -1,
            SQLITE_TRANSIENT) == SQLITE_OK;
    const int renamed = bound ? sqlite3_step(rename) : SQLITE_ERROR;
    const int affected = sqlite3_changes(database_.handle());
    sqlite3_finalize(rename);
    if (renamed != SQLITE_DONE || affected != 1)
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::nativeIdentityConflict);
    }

    NativeTimerBinding durable = next;
    durable.bindingRevision = std::to_string(revision + 1);
    if (!detail::nativeBindingUpdate(database_, durable, revision) ||
        !database_.execute("COMMIT;"))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    return bindingResult(NativeTimerBindingRepositoryStatus::ok, durable);
}

}
