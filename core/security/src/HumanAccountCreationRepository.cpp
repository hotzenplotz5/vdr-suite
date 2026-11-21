#include "HumanAccountCreationRepository.h"

#include "Database.h"

#include <sqlite3.h>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
               statement, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* text = sqlite3_column_text(statement, column);
    return text == nullptr
        ? std::string()
        : std::string(reinterpret_cast<const char*>(text));
}
}

HumanAccountCreationRepository::HumanAccountCreationRepository(Database& database)
    : database_(database)
{
}

bool HumanAccountCreationRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS security_human_account_create_idempotency ("
        "actor_id TEXT NOT NULL,"
        "idempotency_key TEXT NOT NULL,"
        "login_name TEXT NOT NULL,"
        "display_name TEXT NOT NULL,"
        "account_id TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(actor_id, idempotency_key),"
        "FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id),"
        "FOREIGN KEY(account_id) REFERENCES security_human_accounts(account_id)"
        ");");
}

std::optional<HumanAccountCreateIdempotencyRecord>
HumanAccountCreationRepository::find(
    const std::string& actorId,
    const std::string& idempotencyKey) const
{
    if (actorId.empty() || idempotencyKey.empty())
    {
        return std::nullopt;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT actor_id, idempotency_key, login_name, display_name, account_id "
        "FROM security_human_account_create_idempotency "
        "WHERE actor_id = ? AND idempotency_key = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    if (!bindText(statement, 1, actorId) ||
        !bindText(statement, 2, idempotencyKey))
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::optional<HumanAccountCreateIdempotencyRecord> result;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        HumanAccountCreateIdempotencyRecord record;
        record.actorId = columnText(statement, 0);
        record.idempotencyKey = columnText(statement, 1);
        record.loginName = columnText(statement, 2);
        record.displayName = columnText(statement, 3);
        record.accountId = columnText(statement, 4);
        result = record;
    }

    sqlite3_finalize(statement);
    return result;
}

bool HumanAccountCreationRepository::insertInActiveTransaction(
    const HumanAccountCreateIdempotencyRecord& record)
{
    if (!database_.transactionActive() ||
        record.actorId.empty() ||
        record.idempotencyKey.empty() ||
        record.loginName.empty() ||
        record.displayName.empty() ||
        record.accountId.empty())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO security_human_account_create_idempotency "
        "(actor_id, idempotency_key, login_name, display_name, account_id) "
        "VALUES (?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound =
        bindText(statement, 1, record.actorId) &&
        bindText(statement, 2, record.idempotencyKey) &&
        bindText(statement, 3, record.loginName) &&
        bindText(statement, 4, record.displayName) &&
        bindText(statement, 5, record.accountId);
    const int result = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}
