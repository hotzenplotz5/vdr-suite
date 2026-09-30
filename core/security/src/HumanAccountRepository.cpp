#include "HumanAccountRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>

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

bool safeAccountPart(
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

bool readAccount(
    sqlite3_stmt* statement,
    HumanAccountRecord& account)
{
    if (columnText(statement, 4) != "user")
    {
        return false;
    }

    account.accountId = columnText(statement, 0);
    account.actorId = columnText(statement, 1);
    account.displayName = columnText(statement, 2);

    const bool accountActive =
        sqlite3_column_int(statement, 3) != 0;
    const bool actorActive =
        sqlite3_column_int(statement, 5) != 0;
    const bool actorRevoked =
        sqlite3_column_int(statement, 6) != 0;

    account.active =
        accountActive && actorActive && !actorRevoked;

    return !account.accountId.empty() &&
        !account.actorId.empty() &&
        !account.displayName.empty();
}

constexpr const char* AccountSelect =
    "SELECT account.account_id, account.actor_id, "
    "account.display_name, account.active, "
    "actor.actor_type, actor.active, actor.revoked_at <> '' "
    "FROM security_human_accounts AS account "
    "JOIN security_actors AS actor "
    "ON actor.actor_id = account.actor_id ";
}

HumanAccountRepository::HumanAccountRepository(Database& database)
    : database_(database)
{
}

bool HumanAccountRepository::ensureSchema()
{
    return database_.execute(
               "CREATE TABLE IF NOT EXISTS security_human_accounts ("
               "account_id TEXT PRIMARY KEY,"
               "actor_id TEXT NOT NULL UNIQUE,"
               "display_name TEXT NOT NULL,"
               "active INTEGER NOT NULL DEFAULT 1,"
               "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id)"
               ");") &&
        database_.execute(
               "CREATE UNIQUE INDEX IF NOT EXISTS "
               "idx_security_human_accounts_actor "
               "ON security_human_accounts(actor_id);") &&
        database_.execute(
               "CREATE TRIGGER IF NOT EXISTS "
               "security_human_accounts_require_user_insert "
               "BEFORE INSERT ON security_human_accounts "
               "WHEN NOT EXISTS ("
               "SELECT 1 FROM security_actors "
               "WHERE actor_id = NEW.actor_id "
               "AND actor_type = 'user'"
               ") BEGIN "
               "SELECT RAISE(ABORT, "
               "'human account requires user actor'); "
               "END;") &&
        database_.execute(
               "CREATE TRIGGER IF NOT EXISTS "
               "security_human_accounts_require_user_update "
               "BEFORE UPDATE OF actor_id ON security_human_accounts "
               "WHEN NOT EXISTS ("
               "SELECT 1 FROM security_actors "
               "WHERE actor_id = NEW.actor_id "
               "AND actor_type = 'user'"
               ") BEGIN "
               "SELECT RAISE(ABORT, "
               "'human account requires user actor'); "
               "END;") &&
        database_.execute(
               "CREATE TRIGGER IF NOT EXISTS "
               "security_human_accounts_preserve_user_actor "
               "BEFORE UPDATE OF actor_type ON security_actors "
               "WHEN NEW.actor_type <> 'user' "
               "AND EXISTS ("
               "SELECT 1 FROM security_human_accounts "
               "WHERE actor_id = OLD.actor_id"
               ") BEGIN "
               "SELECT RAISE(ABORT, "
               "'bound human account actor must remain user'); "
               "END;");
}

bool HumanAccountRepository::ensureAccountInActiveTransaction(
    const std::string& accountId,
    const std::string& actorId,
    const std::string& displayName)
{
    if (!database_.transactionActive() ||
        !safeAccountPart(accountId, 128) ||
        !safeAccountPart(actorId, 128) ||
        !safeAccountPart(displayName, 256))
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT OR IGNORE INTO security_human_accounts "
        "(account_id, actor_id, display_name) "
        "VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound =
        bindText(statement, 1, accountId) &&
        bindText(statement, 2, actorId) &&
        bindText(statement, 3, displayName);
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
    {
        return false;
    }

    const HumanAccountLookupResult stored =
        findByAccountId(accountId);
    return stored.status == HumanAccountRepositoryStatus::ok &&
        stored.account.actorId == actorId &&
        stored.account.displayName == displayName &&
        stored.account.active;
}

HumanAccountLookupResult
HumanAccountRepository::findByAccountId(
    const std::string& accountId) const
{
    HumanAccountLookupResult result;
    if (accountId.empty())
    {
        result.status = HumanAccountRepositoryStatus::invalid;
        return result;
    }

    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string(AccountSelect) +
        "WHERE account.account_id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        result.status = HumanAccountRepositoryStatus::storageError;
        return result;
    }

    if (!bindText(statement, 1, accountId))
    {
        sqlite3_finalize(statement);
        result.status = HumanAccountRepositoryStatus::storageError;
        return result;
    }

    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        result.status = readAccount(statement, result.account)
            ? HumanAccountRepositoryStatus::ok
            : HumanAccountRepositoryStatus::storageError;
    }
    else if (step == SQLITE_DONE)
    {
        result.status = HumanAccountRepositoryStatus::notFound;
    }
    else
    {
        result.status = HumanAccountRepositoryStatus::storageError;
    }

    sqlite3_finalize(statement);
    return result;
}

HumanAccountListResult HumanAccountRepository::listAll() const
{
    HumanAccountListResult result;
    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string(AccountSelect) +
        "ORDER BY account.account_id ASC;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        result.status = HumanAccountRepositoryStatus::storageError;
        return result;
    }

    for (;;)
    {
        const int step = sqlite3_step(statement);
        if (step == SQLITE_DONE)
        {
            result.status = HumanAccountRepositoryStatus::ok;
            break;
        }
        if (step != SQLITE_ROW)
        {
            result.accounts.clear();
            result.status = HumanAccountRepositoryStatus::storageError;
            break;
        }

        HumanAccountRecord account;
        if (!readAccount(statement, account))
        {
            result.accounts.clear();
            result.status = HumanAccountRepositoryStatus::storageError;
            break;
        }
        result.accounts.push_back(account);
    }

    sqlite3_finalize(statement);
    return result;
}
