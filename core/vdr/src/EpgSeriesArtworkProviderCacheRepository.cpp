#include "EpgSeriesArtworkProviderCacheRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <string>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

const char* outcomeName(SeriesArtworkProviderCacheOutcome outcome)
{
    switch (outcome)
    {
    case SeriesArtworkProviderCacheOutcome::NotFound:
        return "not-found";
    case SeriesArtworkProviderCacheOutcome::TemporarilyUnavailable:
        return "temporarily-unavailable";
    case SeriesArtworkProviderCacheOutcome::None:
        break;
    }
    return "none";
}

SeriesArtworkProviderCacheOutcome parseOutcome(const unsigned char* value)
{
    if (value == nullptr)
    {
        return SeriesArtworkProviderCacheOutcome::None;
    }
    const std::string text(reinterpret_cast<const char*>(value));
    if (text == "not-found")
    {
        return SeriesArtworkProviderCacheOutcome::NotFound;
    }
    if (text == "temporarily-unavailable")
    {
        return SeriesArtworkProviderCacheOutcome::TemporarilyUnavailable;
    }
    return SeriesArtworkProviderCacheOutcome::None;
}
}

EpgSeriesArtworkProviderCacheRepository::EpgSeriesArtworkProviderCacheRepository(
    Database& database)
    : database_(database)
{
}

bool EpgSeriesArtworkProviderCacheRepository::ensureSchema()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return ensureSchemaLocked();
}

bool EpgSeriesArtworkProviderCacheRepository::ensureSchemaLocked()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS epg_series_artwork_provider_cache ("
        "provider TEXT NOT NULL,"
        "identity_provider TEXT NOT NULL,"
        "identity_value TEXT NOT NULL,"
        "outcome TEXT NOT NULL,"
        "expires_at INTEGER NOT NULL,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(provider,identity_provider,identity_value),"
        "CHECK(outcome IN ('not-found','temporarily-unavailable'))"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_epg_series_artwork_provider_cache_expiry "
        "ON epg_series_artwork_provider_cache(expires_at);"
    );
}

SeriesArtworkProviderCacheEntry
EpgSeriesArtworkProviderCacheRepository::find(
    const SeriesArtworkProviderCacheKey& key,
    long long now)
{
    std::lock_guard<std::mutex> lock(mutex_);
    SeriesArtworkProviderCacheEntry entry;
    if (!key.valid() || now < 0 || !ensureSchemaLocked())
    {
        return entry;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT outcome,expires_at FROM epg_series_artwork_provider_cache "
        "WHERE provider=? AND identity_provider=? AND identity_value=?;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return entry;
    }

    const bool bound =
        bindText(statement, 1, key.provider) &&
        bindText(statement, 2, key.identityProvider) &&
        bindText(statement, 3, key.identityValue);
    if (bound && sqlite3_step(statement) == SQLITE_ROW)
    {
        entry.outcome = parseOutcome(sqlite3_column_text(statement, 0));
        entry.expiresAt = sqlite3_column_int64(statement, 1);
    }
    sqlite3_finalize(statement);

    if (!entry.active(now) && entry.outcome != SeriesArtworkProviderCacheOutcome::None)
    {
        sqlite3_stmt* removeStatement = nullptr;
        const char* removeSql =
            "DELETE FROM epg_series_artwork_provider_cache "
            "WHERE provider=? AND identity_provider=? AND identity_value=?;";
        if (sqlite3_prepare_v2(
                database_.handle(), removeSql, -1,
                &removeStatement, nullptr) == SQLITE_OK)
        {
            const bool removeBound =
                bindText(removeStatement, 1, key.provider) &&
                bindText(removeStatement, 2, key.identityProvider) &&
                bindText(removeStatement, 3, key.identityValue);
            if (removeBound)
            {
                sqlite3_step(removeStatement);
            }
            sqlite3_finalize(removeStatement);
        }
        entry = SeriesArtworkProviderCacheEntry{};
    }

    return entry;
}

bool EpgSeriesArtworkProviderCacheRepository::store(
    const SeriesArtworkProviderCacheKey& key,
    SeriesArtworkProviderCacheOutcome outcome,
    long long expiresAt)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!key.valid() ||
        outcome == SeriesArtworkProviderCacheOutcome::None ||
        expiresAt <= 0 ||
        !ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO epg_series_artwork_provider_cache("
        "provider,identity_provider,identity_value,outcome,expires_at,updated_at) "
        "VALUES(?,?,?,?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(provider,identity_provider,identity_value) DO UPDATE SET "
        "outcome=excluded.outcome,expires_at=excluded.expires_at,"
        "updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound =
        bindText(statement, 1, key.provider) &&
        bindText(statement, 2, key.identityProvider) &&
        bindText(statement, 3, key.identityValue) &&
        bindText(statement, 4, outcomeName(outcome)) &&
        sqlite3_bind_int64(statement, 5, expiresAt) == SQLITE_OK;
    const bool ok = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

bool EpgSeriesArtworkProviderCacheRepository::remove(
    const SeriesArtworkProviderCacheKey& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!key.valid() || !ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM epg_series_artwork_provider_cache "
        "WHERE provider=? AND identity_provider=? AND identity_value=?;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool bound =
        bindText(statement, 1, key.provider) &&
        bindText(statement, 2, key.identityProvider) &&
        bindText(statement, 3, key.identityValue);
    const bool ok = bound && sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}
