#include "PublicRecordingIdentityRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <string>

namespace
{
bool valid(const std::string& backendId, const std::string& nativeId)
{
    return !backendId.empty() && backendId.size() <= 128 &&
        !nativeId.empty() && nativeId.size() <= 4096;
}

bool bind(sqlite3_stmt* statement, int index, const std::string& text)
{
    return sqlite3_bind_text(statement, index, text.data(),
               static_cast<int>(text.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

std::optional<std::string> lookup(
    sqlite3* database,
    const std::string& backendId,
    const std::string& nativeId)
{
    sqlite3_stmt* statement = nullptr;
    constexpr const char* query =
        "SELECT public_id FROM public_recording_identity "
        "WHERE backend_id=?1 AND native_id=?2;";
    if (sqlite3_prepare_v2(database, query, -1, &statement, nullptr) != SQLITE_OK)
        return std::nullopt;

    std::optional<std::string> id;
    if (bind(statement, 1, backendId) && bind(statement, 2, nativeId) &&
        sqlite3_step(statement) == SQLITE_ROW)
    {
        const auto* value = sqlite3_column_text(statement, 0);
        if (value != nullptr) id = reinterpret_cast<const char*>(value);
    }
    sqlite3_finalize(statement);
    return id;
}
} // namespace

PublicRecordingIdentityRepository::PublicRecordingIdentityRepository(
    Database& database)
    : database_(database)
{
}

bool PublicRecordingIdentityRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS public_recording_identity ("
        "public_id TEXT PRIMARY KEY NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "native_id TEXT NOT NULL,"
        "UNIQUE (backend_id, native_id)"
        ");");
}

std::optional<std::string> PublicRecordingIdentityRepository::find(
    const std::string& backendId,
    const std::string& backendNativeId) const
{
    if (!valid(backendId, backendNativeId)) return std::nullopt;
    auto lease = database_.acquireTransactionLease();
    return lookup(database_.handle(), backendId, backendNativeId);
}

std::optional<std::string> PublicRecordingIdentityRepository::resolveOrCreate(
    const std::string& backendId,
    const std::string& backendNativeId)
{
    if (!valid(backendId, backendNativeId)) return std::nullopt;
    auto lease = database_.acquireTransactionLease();
    if (const auto existing = lookup(database_.handle(), backendId, backendNativeId))
        return existing;

    sqlite3_stmt* statement = nullptr;
    constexpr const char* sql =
        "INSERT OR IGNORE INTO public_recording_identity "
        "(public_id, backend_id, native_id) "
        "VALUES('rec_' || lower(hex(randomblob(16))), ?1, ?2);";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK) return std::nullopt;

    const bool success = bind(statement, 1, backendId) &&
        bind(statement, 2, backendNativeId) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return success ? lookup(database_.handle(), backendId, backendNativeId)
                   : std::nullopt;
}

bool PublicRecordingIdentityRepository::rebindAfterVerifiedMove(
    const std::string& backendId,
    const std::string& previousNativeId,
    const std::string& nextNativeId)
{
    if (!valid(backendId, previousNativeId) ||
        !valid(backendId, nextNativeId) ||
        previousNativeId == nextNativeId) return false;
    auto lease = database_.acquireTransactionLease();

    sqlite3_stmt* statement = nullptr;
    constexpr const char* sql =
        "UPDATE OR IGNORE public_recording_identity "
        "SET native_id=?3 WHERE backend_id=?1 AND native_id=?2;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK) return false;

    const bool success = bind(statement, 1, backendId) &&
        bind(statement, 2, previousNativeId) &&
        bind(statement, 3, nextNativeId) &&
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(database_.handle()) == 1;
    sqlite3_finalize(statement);
    return success;
}
