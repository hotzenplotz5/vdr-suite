#include "EpgSearchNativeFuzzyCapabilityRepository.h"

#include "Database.h"

#include <sqlite3.h>

namespace
{
    int boolToInt(bool value)
    {
        return value ? 1 : 0;
    }

    bool intToBool(sqlite3_stmt* stmt, int column)
    {
        return sqlite3_column_int(stmt, column) != 0;
    }
}

EpgSearchNativeFuzzyCapabilityRepository::EpgSearchNativeFuzzyCapabilityRepository(
    Database& database)
    : database_(database)
{
}

bool EpgSearchNativeFuzzyCapabilityRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS epgsearch_native_fuzzy_capability_probes ("
        "backend_id TEXT PRIMARY KEY,"
        "create_accepted INTEGER NOT NULL DEFAULT 0,"
        "readback_available INTEGER NOT NULL DEFAULT 0,"
        "mode_preserved INTEGER NOT NULL DEFAULT 0,"
        "tolerance_preserved INTEGER NOT NULL DEFAULT 0,"
        "cleanup_succeeded INTEGER NOT NULL DEFAULT 0,"
        "updated_at TEXT DEFAULT CURRENT_TIMESTAMP"
        ");");
}

bool EpgSearchNativeFuzzyCapabilityRepository::save(
    const std::string& backendId,
    const EpgSearchNativeFuzzyCapabilityProbeResult& result)
{
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "INSERT INTO epgsearch_native_fuzzy_capability_probes ("
        "backend_id, "
        "create_accepted, "
        "readback_available, "
        "mode_preserved, "
        "tolerance_preserved, "
        "cleanup_succeeded, "
        "updated_at"
        ") VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id) DO UPDATE SET "
        "create_accepted=excluded.create_accepted, "
        "readback_available=excluded.readback_available, "
        "mode_preserved=excluded.mode_preserved, "
        "tolerance_preserved=excluded.tolerance_preserved, "
        "cleanup_succeeded=excluded.cleanup_succeeded, "
        "updated_at=CURRENT_TIMESTAMP;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, backendId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, boolToInt(result.createAccepted));
    sqlite3_bind_int(stmt, 3, boolToInt(result.readbackAvailable));
    sqlite3_bind_int(stmt, 4, boolToInt(result.modePreserved));
    sqlite3_bind_int(stmt, 5, boolToInt(result.tolerancePreserved));
    sqlite3_bind_int(stmt, 6, boolToInt(result.cleanupSucceeded));

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::optional<EpgSearchNativeFuzzyCapabilityProbeResult> EpgSearchNativeFuzzyCapabilityRepository::load(
    const std::string& backendId) const
{
    sqlite3_stmt* stmt = nullptr;

    const char* sql =
        "SELECT "
        "create_accepted, "
        "readback_available, "
        "mode_preserved, "
        "tolerance_preserved, "
        "cleanup_succeeded "
        "FROM epgsearch_native_fuzzy_capability_probes "
        "WHERE backend_id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, backendId.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<EpgSearchNativeFuzzyCapabilityProbeResult> result;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        EpgSearchNativeFuzzyCapabilityProbeResult probeResult;
        probeResult.createAccepted = intToBool(stmt, 0);
        probeResult.readbackAvailable = intToBool(stmt, 1);
        probeResult.modePreserved = intToBool(stmt, 2);
        probeResult.tolerancePreserved = intToBool(stmt, 3);
        probeResult.cleanupSucceeded = intToBool(stmt, 4);
        result = probeResult;
    }

    sqlite3_finalize(stmt);

    return result;
}
