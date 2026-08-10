#include "NativeTimerBindingRepositoryStorage.h"

#include "Database.h"

#include <sqlite3.h>

#include <limits>
#include <string>

namespace vdrsuite::timers::detail
{
namespace
{
const char* kSelectColumns =
    "native_timer_binding_id,binding_revision,backend_id,backend_generation,"
    "backend_native_timer_id,timer_assignment_id,ownership,"
    "observed_fingerprint,observed_channel_id,observed_event_id,"
    "observed_title,observed_directory,observed_day,observed_weekdays,"
    "observed_start_time,observed_end_time,observed_flags,observed_priority,"
    "observed_lifetime,observed_enabled,observed_vps,observed_recording,"
    "observed_pending,last_observed_at,last_verified_operation_id,"
    "missing_since,drift_state";

std::string text(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    if (!value) return {};
    return std::string(
        reinterpret_cast<const char*>(value),
        static_cast<std::size_t>(sqlite3_column_bytes(statement, column)));
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    return sqlite3_bind_text(
        statement, index, value.data(), static_cast<int>(value.size()),
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool ownership(const std::string& value, NativeTimerBindingOwnership& out)
{
    if (value == "managed") out = NativeTimerBindingOwnership::managed;
    else if (value == "adopted") out = NativeTimerBindingOwnership::adopted;
    else if (value == "external") out = NativeTimerBindingOwnership::external;
    else if (value == "orphaned_managed") out = NativeTimerBindingOwnership::orphanedManaged;
    else if (value == "ambiguous") out = NativeTimerBindingOwnership::ambiguous;
    else return false;
    return true;
}

bool drift(const std::string& value, NativeTimerBindingDriftState& out)
{
    if (value == "none") out = NativeTimerBindingDriftState::none;
    else if (value == "expected_transition") out = NativeTimerBindingDriftState::expectedTransition;
    else if (value == "external_field_change") out = NativeTimerBindingDriftState::externalFieldChange;
    else if (value == "external_disable") out = NativeTimerBindingDriftState::externalDisable;
    else if (value == "external_delete") out = NativeTimerBindingDriftState::externalDelete;
    else if (value == "native_identity_changed") out = NativeTimerBindingDriftState::nativeIdentityChanged;
    else if (value == "ambiguous") out = NativeTimerBindingDriftState::ambiguous;
    else return false;
    return true;
}

bool read(sqlite3_stmt* statement, NativeTimerBinding& b)
{
    const sqlite3_int64 revision = sqlite3_column_int64(statement, 1);
    const sqlite3_int64 generation = sqlite3_column_int64(statement, 3);
    if (revision <= 0 || generation <= 0) return false;

    b.nativeTimerBindingId = text(statement, 0);
    b.bindingRevision = std::to_string(revision);
    b.backendId = text(statement, 2);
    b.backendGeneration = static_cast<std::uint64_t>(generation);
    b.backendNativeTimerId = text(statement, 4);
    b.timerAssignmentId = text(statement, 5);
    if (!ownership(text(statement, 6), b.ownership)) return false;
    b.observedFingerprint = text(statement, 7);
    b.observedState.channelId = text(statement, 8);
    b.observedState.eventId = text(statement, 9);
    b.observedState.title = text(statement, 10);
    b.observedState.directory = text(statement, 11);
    b.observedState.day = text(statement, 12);
    b.observedState.weekdays = text(statement, 13);
    b.observedState.startTime = text(statement, 14);
    b.observedState.endTime = text(statement, 15);
    b.observedState.flags = sqlite3_column_int(statement, 16);
    b.observedState.priority = sqlite3_column_int(statement, 17);
    b.observedState.lifetime = sqlite3_column_int(statement, 18);
    b.observedState.enabled = sqlite3_column_int(statement, 19) != 0;
    b.observedState.vps = sqlite3_column_int(statement, 20) != 0;
    b.observedState.recording = sqlite3_column_int(statement, 21) != 0;
    b.observedState.pending = sqlite3_column_int(statement, 22) != 0;
    b.lastObservedAt = sqlite3_column_int64(statement, 23);
    b.lastVerifiedOperationId = text(statement, 24);
    b.missingSince = sqlite3_column_int64(statement, 25);
    if (!drift(text(statement, 26), b.driftState)) return false;
    return nativeTimerBindingValid(b);
}

bool selectOne(
    Database& database,
    const std::string& where,
    const std::string& first,
    const std::string* second,
    NativeTimerBinding& binding,
    bool& found)
{
    found = false;
    sqlite3_stmt* statement = nullptr;
    const std::string sql = std::string("SELECT ") + kSelectColumns +
        " FROM native_timer_bindings WHERE " + where + " LIMIT 1;";
    if (sqlite3_prepare_v2(database.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK)
        return false;
    if (!bindText(statement, 1, first) || (second && !bindText(statement, 2, *second)))
    {
        sqlite3_finalize(statement);
        return false;
    }
    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        found = read(statement, binding);
        sqlite3_finalize(statement);
        return found;
    }
    const bool ok = step == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}
}

bool nativeBindingSelectById(
    Database& database, const std::string& id,
    NativeTimerBinding& binding, bool& found)
{
    return selectOne(
        database, "native_timer_binding_id=?", id, nullptr, binding, found);
}

bool nativeBindingSelectByNativeIdentity(
    Database& database, const std::string& backendId,
    const std::string& nativeId, NativeTimerBinding& binding, bool& found)
{
    return selectOne(
        database, "backend_id=? AND backend_native_timer_id=?",
        backendId, &nativeId, binding, found);
}

bool nativeBindingSelectManagedForAssignment(
    Database& database, const std::string& assignmentId,
    NativeTimerBinding& binding, bool& found)
{
    return selectOne(
        database,
        "timer_assignment_id=? AND ownership IN ('managed','adopted')",
        assignmentId, nullptr, binding, found);
}

bool nativeBindingListForAssignment(
    Database& database, const std::string& assignmentId,
    std::vector<NativeTimerBinding>& bindings)
{
    bindings.clear();
    sqlite3_stmt* statement = nullptr;
    const std::string sql = std::string("SELECT ") + kSelectColumns +
        " FROM native_timer_bindings WHERE timer_assignment_id=?"
        " ORDER BY native_timer_binding_id ASC;";
    if (sqlite3_prepare_v2(database.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, assignmentId))
    {
        if (statement) sqlite3_finalize(statement);
        return false;
    }
    while (true)
    {
        const int step = sqlite3_step(statement);
        if (step == SQLITE_DONE) break;
        if (step != SQLITE_ROW)
        {
            sqlite3_finalize(statement);
            bindings.clear();
            return false;
        }
        NativeTimerBinding binding;
        if (!read(statement, binding))
        {
            sqlite3_finalize(statement);
            bindings.clear();
            return false;
        }
        bindings.push_back(binding);
    }
    sqlite3_finalize(statement);
    return true;
}

}
