#include "NativeTimerBindingRepository.h"

#include "Database.h"
#include "NativeTimerBindingRepositoryStorage.h"

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

NativeTimerBindingRepositoryResult NativeTimerBindingRepository::update(
    const NativeTimerBinding& next,
    const std::string& expectedRevision)
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
    if (!database_.execute("COMMIT;"))
    {
        rollback();
        return statusResult(NativeTimerBindingRepositoryStatus::storageError);
    }
    return bindingResult(NativeTimerBindingRepositoryStatus::ok, durable);
}

}
