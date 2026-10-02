#include "HumanAccountCreationRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace
{
bool bindText(
    sqlite3_stmt* statement,
    int index,
    const std::string& value)
{
    return sqlite3_bind_text(
               statement,
               index,
               value.c_str(),
               -1,
               SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* text = sqlite3_column_text(statement, column);
    return text == nullptr
        ? std::string()
        : std::string(reinterpret_cast<const char*>(text));
}

bool safeText(
    const std::string& value,
    std::size_t maximumLength)
{
    if (value.empty() || value.size() > maximumLength)
    {
        return false;
    }

    return std::none_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return character == '\0' ||
                character == '\r' ||
                character == '\n' ||
                std::iscntrl(character);
        });
}

bool validRecord(
    const HumanAccountCreateIdempotencyRecord& record)
{
    return safeText(record.requestActorId, 128) &&
        safeText(record.idempotencyKey, 160) &&
        safeText(record.loginName, 128) &&
        safeText(record.displayName, 256) &&
        safeText(record.accountId, 128);
}

bool sameRequest(
    const HumanAccountCreateIdempotencyRecord& left,
    const HumanAccountCreateIdempotencyRecord& right)
{
    return left.requestActorId == right.requestActorId &&
        left.idempotencyKey == right.idempotencyKey &&
        left.loginName == right.loginName &&
        left.displayName == right.displayName &&
        left.accountId == right.accountId;
}
}

HumanAccountCreationRepository::HumanAccountCreationRepository(
    Database& database)
    : database_(database)
{
}

bool HumanAccountCreationRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS "
        "security_human_account_create_idempotency ("
        "request_actor_id TEXT NOT NULL,"
        "idempotency_key TEXT NOT NULL,"
        "login_name TEXT NOT NULL,"
        "display_name TEXT NOT NULL,"
        "account_id TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(request_actor_id, idempotency_key),"
        "UNIQUE(account_id),"
        "FOREIGN KEY(request_actor_id) REFERENCES security_actors(actor_id),"
        "FOREIGN KEY(account_id) REFERENCES security_human_accounts(account_id)"
        ");");
}

HumanAccountCreateIdempotencyLookupResult
HumanAccountCreationRepository::find(
    const std::string& requestActorId,
    const std::string& idempotencyKey) const
{
    HumanAccountCreateIdempotencyLookupResult result;
    if (!safeText(requestActorId, 128) ||
        !safeText(idempotencyKey, 160))
    {
        result.status = HumanAccountCreateIdempotencyStatus::invalid;
        return result;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT request_actor_id, idempotency_key, login_name, "
        "display_name, account_id "
        "FROM security_human_account_create_idempotency "
        "WHERE request_actor_id = ? AND idempotency_key = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return result;
    }

    if (!bindText(statement, 1, requestActorId) ||
        !bindText(statement, 2, idempotencyKey))
    {
        sqlite3_finalize(statement);
        return result;
    }

    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        result.record.requestActorId = columnText(statement, 0);
        result.record.idempotencyKey = columnText(statement, 1);
        result.record.loginName = columnText(statement, 2);
        result.record.displayName = columnText(statement, 3);
        result.record.accountId = columnText(statement, 4);
        result.status = validRecord(result.record)
            ? HumanAccountCreateIdempotencyStatus::ok
            : HumanAccountCreateIdempotencyStatus::storageError;
    }
    else if (step == SQLITE_DONE)
    {
        result.status = HumanAccountCreateIdempotencyStatus::notFound;
    }

    sqlite3_finalize(statement);
    return result;
}

HumanAccountCreateIdempotencyStatus
HumanAccountCreationRepository::recordInActiveTransaction(
    const HumanAccountCreateIdempotencyRecord& record)
{
    if (!database_.transactionActive() || !validRecord(record))
    {
        return HumanAccountCreateIdempotencyStatus::invalid;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT OR IGNORE INTO security_human_account_create_idempotency "
        "(request_actor_id, idempotency_key, login_name, display_name, account_id) "
        "VALUES (?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return HumanAccountCreateIdempotencyStatus::storageError;
    }

    const bool bound =
        bindText(statement, 1, record.requestActorId) &&
        bindText(statement, 2, record.idempotencyKey) &&
        bindText(statement, 3, record.loginName) &&
        bindText(statement, 4, record.displayName) &&
        bindText(statement, 5, record.accountId);
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
    {
        return HumanAccountCreateIdempotencyStatus::storageError;
    }
    if (changed == 1)
    {
        return HumanAccountCreateIdempotencyStatus::ok;
    }

    const HumanAccountCreateIdempotencyLookupResult stored =
        find(record.requestActorId, record.idempotencyKey);
    if (stored.status != HumanAccountCreateIdempotencyStatus::ok)
    {
        return HumanAccountCreateIdempotencyStatus::storageError;
    }
    return sameRequest(stored.record, record)
        ? HumanAccountCreateIdempotencyStatus::ok
        : HumanAccountCreateIdempotencyStatus::conflict;
}
