#include "NativeTimerBindingRepositoryStorage.h"

#include "Database.h"

#include <sqlite3.h>

#include <limits>
#include <string>

namespace vdrsuite::timers::detail
{
namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    return sqlite3_bind_text(
        statement, index, value.data(), static_cast<int>(value.size()),
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool revision(const std::string& token, std::int64_t& value)
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

bool generation(std::uint64_t value)
{
    return value <= static_cast<std::uint64_t>(
        std::numeric_limits<sqlite3_int64>::max());
}

bool bindAll(sqlite3_stmt* s, const NativeTimerBinding& b)
{
    std::int64_t rev = 0;
    if (!revision(b.bindingRevision, rev) || !generation(b.backendGeneration))
        return false;
    int i = 1;
    return bindText(s, i++, b.nativeTimerBindingId)
        && sqlite3_bind_int64(s, i++, rev) == SQLITE_OK
        && bindText(s, i++, b.backendId)
        && sqlite3_bind_int64(s, i++, static_cast<sqlite3_int64>(b.backendGeneration)) == SQLITE_OK
        && bindText(s, i++, b.backendNativeTimerId)
        && bindText(s, i++, b.timerAssignmentId)
        && bindText(s, i++, nativeTimerBindingOwnershipName(b.ownership))
        && bindText(s, i++, b.observedFingerprint)
        && bindText(s, i++, b.observedState.channelId)
        && bindText(s, i++, b.observedState.eventId)
        && bindText(s, i++, b.observedState.title)
        && bindText(s, i++, b.observedState.directory)
        && bindText(s, i++, b.observedState.day)
        && bindText(s, i++, b.observedState.weekdays)
        && bindText(s, i++, b.observedState.startTime)
        && bindText(s, i++, b.observedState.endTime)
        && sqlite3_bind_int(s, i++, b.observedState.flags) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.priority) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.lifetime) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.enabled ? 1 : 0) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.vps ? 1 : 0) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.recording ? 1 : 0) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.pending ? 1 : 0) == SQLITE_OK
        && sqlite3_bind_int64(s, i++, b.lastObservedAt) == SQLITE_OK
        && bindText(s, i++, b.lastVerifiedOperationId)
        && sqlite3_bind_int64(s, i++, b.missingSince) == SQLITE_OK
        && bindText(s, i, nativeTimerBindingDriftStateName(b.driftState));
}

bool bindMutable(
    sqlite3_stmt* s, const NativeTimerBinding& b,
    const std::string& id, std::int64_t expected)
{
    std::int64_t rev = 0;
    if (!revision(b.bindingRevision, rev) || !generation(b.backendGeneration))
        return false;
    int i = 1;
    return sqlite3_bind_int64(s, i++, rev) == SQLITE_OK
        && sqlite3_bind_int64(s, i++, static_cast<sqlite3_int64>(b.backendGeneration)) == SQLITE_OK
        && bindText(s, i++, b.observedFingerprint)
        && bindText(s, i++, b.observedState.channelId)
        && bindText(s, i++, b.observedState.eventId)
        && bindText(s, i++, b.observedState.title)
        && bindText(s, i++, b.observedState.directory)
        && bindText(s, i++, b.observedState.day)
        && bindText(s, i++, b.observedState.weekdays)
        && bindText(s, i++, b.observedState.startTime)
        && bindText(s, i++, b.observedState.endTime)
        && sqlite3_bind_int(s, i++, b.observedState.flags) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.priority) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.lifetime) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.enabled ? 1 : 0) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.vps ? 1 : 0) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.recording ? 1 : 0) == SQLITE_OK
        && sqlite3_bind_int(s, i++, b.observedState.pending ? 1 : 0) == SQLITE_OK
        && sqlite3_bind_int64(s, i++, b.lastObservedAt) == SQLITE_OK
        && bindText(s, i++, b.lastVerifiedOperationId)
        && sqlite3_bind_int64(s, i++, b.missingSince) == SQLITE_OK
        && bindText(s, i++, nativeTimerBindingDriftStateName(b.driftState))
        && bindText(s, i++, id)
        && sqlite3_bind_int64(s, i, expected) == SQLITE_OK;
}
}

bool nativeBindingInsert(Database& database, const NativeTimerBinding& binding)
{
    const char* sql =
        "INSERT INTO native_timer_bindings ("
        "native_timer_binding_id,binding_revision,backend_id,backend_generation,"
        "backend_native_timer_id,timer_assignment_id,ownership,observed_fingerprint,"
        "observed_channel_id,observed_event_id,observed_title,observed_directory,"
        "observed_day,observed_weekdays,observed_start_time,observed_end_time,"
        "observed_flags,observed_priority,observed_lifetime,observed_enabled,"
        "observed_vps,observed_recording,observed_pending,last_observed_at,"
        "last_verified_operation_id,missing_since,drift_state)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const bool ok = bindAll(statement, binding) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

bool nativeBindingUpdate(
    Database& database, const NativeTimerBinding& binding,
    std::int64_t expectedRevision)
{
    const char* sql =
        "UPDATE native_timer_bindings SET binding_revision=?,backend_generation=?,"
        "observed_fingerprint=?,observed_channel_id=?,observed_event_id=?,"
        "observed_title=?,observed_directory=?,observed_day=?,observed_weekdays=?,"
        "observed_start_time=?,observed_end_time=?,observed_flags=?,observed_priority=?,"
        "observed_lifetime=?,observed_enabled=?,observed_vps=?,observed_recording=?,"
        "observed_pending=?,last_observed_at=?,last_verified_operation_id=?,"
        "missing_since=?,drift_state=?"
        " WHERE native_timer_binding_id=? AND binding_revision=?;";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const bool ok = bindMutable(
            statement, binding, binding.nativeTimerBindingId, expectedRevision)
        && sqlite3_step(statement) == SQLITE_DONE
        && sqlite3_changes(database.handle()) == 1;
    sqlite3_finalize(statement);
    return ok;
}

}
