#include "PublicSearchTimerIdentityRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <optional>
#include <string>

namespace
{
bool valid(const std::string& backend, const std::string& native)
{
    return !backend.empty() && backend.size() <= 128U &&
        !native.empty() && native.size() <= 64U;
}

bool bind(sqlite3_stmt* stmt, int index, const std::string& value)
{
    return sqlite3_bind_text(stmt, index, value.data(),
        static_cast<int>(value.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

std::optional<std::string> find(
    sqlite3* db, const std::string& backend, const std::string& native)
{
    sqlite3_stmt* stmt = nullptr;
    constexpr const char* sql =
        "SELECT public_id FROM public_search_timer_identity "
        "WHERE backend_id=?1 AND native_id=?2;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;
    std::optional<std::string> id;
    if (bind(stmt, 1, backend) && bind(stmt, 2, native) &&
        sqlite3_step(stmt) == SQLITE_ROW)
    {
        const auto* value = sqlite3_column_text(stmt, 0);
        if (value) id = reinterpret_cast<const char*>(value);
    }
    sqlite3_finalize(stmt);
    return id;
}
} // namespace

PublicSearchTimerIdentityRepository::PublicSearchTimerIdentityRepository(
    Database& db)
    : database_(db)
{
}

bool PublicSearchTimerIdentityRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS public_search_timer_identity ("
        "public_id TEXT PRIMARY KEY NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "native_id TEXT NOT NULL,"
        "UNIQUE (backend_id, native_id)"
        ");");
}

std::optional<std::string>
PublicSearchTimerIdentityRepository::resolveOrCreate(
    const std::string& backend, const std::string& native)
{
    if (!valid(backend, native)) return std::nullopt;
    auto lease = database_.acquireTransactionLease();
    if (const auto id = find(database_.handle(), backend, native))
        return id;
    sqlite3_stmt* stmt = nullptr;
    constexpr const char* sql =
        "INSERT OR IGNORE INTO public_search_timer_identity "
        "(public_id, backend_id, native_id) "
        "VALUES('st_' || lower(hex(randomblob(16))), ?1, ?2);";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &stmt, nullptr)
        != SQLITE_OK) return std::nullopt;
    const bool success = bind(stmt, 1, backend) &&
        bind(stmt, 2, native) && sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success ? find(database_.handle(), backend, native) :
        std::nullopt;
}
