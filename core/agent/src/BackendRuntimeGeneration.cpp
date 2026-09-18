#include "BackendRuntimeGeneration.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <limits>

namespace
{
bool safeBackendId(const std::string& value)
{
    if (value.empty() || value.size() > 128U) return false;
    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character) {
            return std::isalnum(character) != 0 ||
                character == '-' || character == '_' ||
                character == '.' || character == ':';
        });
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindInt64(sqlite3_stmt* statement, int index, std::int64_t value)
{
    return sqlite3_bind_int64(statement, index, value) == SQLITE_OK;
}

class Transaction
{
public:
    explicit Transaction(Database& database)
        : database_(database),
          lease_(database.acquireTransactionLease()),
          active_(database_.execute("BEGIN IMMEDIATE;"))
    {
    }

    ~Transaction()
    {
        if (active_) database_.execute("ROLLBACK;");
    }

    bool active() const { return active_; }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;")) return false;
        active_ = false;
        return true;
    }

private:
    Database& database_;
    Database::TransactionLease lease_;
    bool active_;
};
}

BackendRuntimeGenerationRepository::BackendRuntimeGenerationRepository(
    Database& database)
    : database_(database)
{
}

bool BackendRuntimeGenerationRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_runtime_generations ("
        "backend_id TEXT PRIMARY KEY,"
        "generation INTEGER NOT NULL,"
        "updated_at INTEGER NOT NULL"
        ");");
}

BackendRuntimeGenerationAllocation
BackendRuntimeGenerationRepository::allocate(
    const std::string& backendId,
    std::int64_t now)
{
    BackendRuntimeGenerationAllocation result;
    Transaction transaction(database_);
    if (!transaction.active())
    {
        result.reasonCode = "backend_generation_transaction_unavailable";
        return result;
    }

    result = allocateInCurrentTransaction(backendId, now);
    if (!result.accepted)
    {
        return result;
    }
    if (!transaction.commit())
    {
        result.accepted = false;
        result.generation = 0;
        result.reasonCode = "backend_generation_commit_failed";
    }
    return result;
}

std::uint64_t BackendRuntimeGenerationRepository::latestGeneration(
    const std::string& backendId) const
{
    if (!safeBackendId(backendId)) return 0;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT MAX(value) FROM ("
        "SELECT generation AS value FROM backend_runtime_generations "
        "WHERE backend_id = ? "
        "UNION ALL "
        "SELECT backend_generation AS value FROM backend_agents "
        "WHERE backend_id = ?"
        ");";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK ||
        !bindText(statement, 1, backendId) ||
        !bindText(statement, 2, backendId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return 0;
    }

    std::uint64_t result = 0;
    if (sqlite3_step(statement) == SQLITE_ROW &&
        sqlite3_column_type(statement, 0) != SQLITE_NULL)
    {
        const std::int64_t value = sqlite3_column_int64(statement, 0);
        if (value > 0)
            result = static_cast<std::uint64_t>(value);
    }
    sqlite3_finalize(statement);
    return result;
}

BackendRuntimeGenerationAllocation
BackendRuntimeGenerationRepository::allocateInCurrentTransaction(
    const std::string& backendId,
    std::int64_t now)
{
    BackendRuntimeGenerationAllocation result;
    if (!safeBackendId(backendId) || now < 0)
    {
        result.reasonCode = "invalid_backend_generation_request";
        return result;
    }

    sqlite3_stmt* lookup = nullptr;
    const char* lookupSql =
        "SELECT MAX(value) FROM ("
        "SELECT generation AS value FROM backend_runtime_generations "
        "WHERE backend_id = ? "
        "UNION ALL "
        "SELECT backend_generation AS value FROM backend_agents "
        "WHERE backend_id = ?"
        ");";

    if (sqlite3_prepare_v2(
            database_.handle(),
            lookupSql,
            -1,
            &lookup,
            nullptr) != SQLITE_OK ||
        !bindText(lookup, 1, backendId) ||
        !bindText(lookup, 2, backendId))
    {
        if (lookup != nullptr) sqlite3_finalize(lookup);
        result.reasonCode = "backend_generation_lookup_failed";
        return result;
    }

    std::int64_t current = 0;
    const int lookupResult = sqlite3_step(lookup);
    if (lookupResult == SQLITE_ROW &&
        sqlite3_column_type(lookup, 0) != SQLITE_NULL)
    {
        current = sqlite3_column_int64(lookup, 0);
    }
    sqlite3_finalize(lookup);

    if (lookupResult != SQLITE_ROW || current < 0 ||
        current == std::numeric_limits<std::int64_t>::max())
    {
        result.reasonCode = current == std::numeric_limits<std::int64_t>::max()
            ? "backend_generation_exhausted"
            : "backend_generation_lookup_failed";
        return result;
    }

    const std::int64_t next = current + 1;
    sqlite3_stmt* update = nullptr;
    const char* updateSql =
        "INSERT INTO backend_runtime_generations "
        "(backend_id, generation, updated_at) VALUES (?, ?, ?) "
        "ON CONFLICT(backend_id) DO UPDATE SET "
        "generation=excluded.generation, updated_at=excluded.updated_at;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            updateSql,
            -1,
            &update,
            nullptr) != SQLITE_OK ||
        !bindText(update, 1, backendId) ||
        !bindInt64(update, 2, next) ||
        !bindInt64(update, 3, now) ||
        sqlite3_step(update) != SQLITE_DONE)
    {
        if (update != nullptr) sqlite3_finalize(update);
        result.reasonCode = "backend_generation_persist_failed";
        return result;
    }
    sqlite3_finalize(update);

    result.accepted = true;
    result.generation = static_cast<std::uint64_t>(next);
    result.reasonCode = "backend_generation_allocated";
    return result;
}
