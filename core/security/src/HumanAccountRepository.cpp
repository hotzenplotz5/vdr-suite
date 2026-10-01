#include "HumanAccountRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
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

bool columnExists(
    Database& database,
    const std::string& tableName,
    const std::string& columnName)
{
    sqlite3_stmt* statement = nullptr;
    const std::string sql = "PRAGMA table_info(" + tableName + ");";
    if (sqlite3_prepare_v2(
            database.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    bool found = false;
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        if (columnText(statement, 1) == columnName)
        {
            found = true;
            break;
        }
    }
    sqlite3_finalize(statement);
    return found;
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
    if (columnText(statement, 5) != "user")
    {
        return false;
    }

    account.accountId = columnText(statement, 0);
    account.actorId = columnText(statement, 1);
    account.displayName = columnText(statement, 2);

    const bool accountActive =
        sqlite3_column_int(statement, 3) != 0;
    const sqlite3_int64 revision =
        sqlite3_column_int64(statement, 4);
    const bool actorActive =
        sqlite3_column_int(statement, 6) != 0;
    const bool actorRevoked =
        sqlite3_column_int(statement, 7) != 0;

    if (revision <= 0)
    {
        return false;
    }

    account.active =
        accountActive && actorActive && !actorRevoked;
    account.revision =
        static_cast<std::uint64_t>(revision);

    return !account.accountId.empty() &&
        !account.actorId.empty() &&
        !account.displayName.empty();
}

constexpr const char* AccountSelect =
    "SELECT account.account_id, account.actor_id, "
    "account.display_name, account.active, account.revision, "
    "actor.actor_type, actor.active, actor.revoked_at <> '' "
    "FROM security_human_accounts AS account "
    "JOIN security_actors AS actor "
    "ON actor.actor_id = account.actor_id ";

HumanAccountRepositoryStatus mutationMissStatus(
    HumanAccountRepository& repository,
    const std::string& accountId,
    std::uint64_t expectedRevision)
{
    const HumanAccountLookupResult current =
        repository.findByAccountId(accountId);
    if (current.status == HumanAccountRepositoryStatus::notFound)
    {
        return HumanAccountRepositoryStatus::notFound;
    }
    if (current.status != HumanAccountRepositoryStatus::ok)
    {
        return HumanAccountRepositoryStatus::storageError;
    }
    return current.account.revision == expectedRevision
        ? HumanAccountRepositoryStatus::storageError
        : HumanAccountRepositoryStatus::revisionConflict;
}
}

HumanAccountRepository::HumanAccountRepository(Database& database)
    : database_(database)
{
}

bool HumanAccountRepository::ensureSchema()
{
    if (!database_.execute(
            "CREATE TABLE IF NOT EXISTS security_human_accounts ("
            "account_id TEXT PRIMARY KEY,"
            "actor_id TEXT NOT NULL UNIQUE,"
            "display_name TEXT NOT NULL,"
            "active INTEGER NOT NULL DEFAULT 1,"
            "revision INTEGER NOT NULL DEFAULT 1,"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id)"
            ");"))
    {
        return false;
    }

    if (!columnExists(
            database_,
            "security_human_accounts",
            "revision") &&
        !database_.execute(
            "ALTER TABLE security_human_accounts "
            "ADD COLUMN revision INTEGER NOT NULL DEFAULT 1;"))
    {
        return false;
    }

    return database_.execute(
               "UPDATE security_human_accounts "
               "SET revision = 1 WHERE revision <= 0;") &&
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
        "(account_id, actor_id, display_name, revision) "
        "VALUES (?, ?, ?, 1);";

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
        stored.account.active &&
        stored.account.revision > 0;
}

HumanAccountRepositoryStatus
HumanAccountRepository::updateDisplayNameInActiveTransaction(
    const std::string& accountId,
    std::uint64_t expectedRevision,
    const std::string& displayName)
{
    if (!database_.transactionActive() ||
        !safeAccountPart(accountId, 128) ||
        !safeAccountPart(displayName, 256) ||
        expectedRevision == 0)
    {
        return HumanAccountRepositoryStatus::invalid;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE security_human_accounts "
        "SET display_name = ?, revision = revision + 1, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE account_id = ? AND revision = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return HumanAccountRepositoryStatus::storageError;
    }

    const bool bound =
        bindText(statement, 1, displayName) &&
        bindText(statement, 2, accountId) &&
        sqlite3_bind_int64(
            statement,
            3,
            static_cast<sqlite3_int64>(expectedRevision)) == SQLITE_OK;
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
    {
        return HumanAccountRepositoryStatus::storageError;
    }
    if (changed == 1)
    {
        return HumanAccountRepositoryStatus::ok;
    }
    return mutationMissStatus(*this, accountId, expectedRevision);
}

HumanAccountRepositoryStatus
HumanAccountRepository::setActiveInActiveTransaction(
    const std::string& accountId,
    std::uint64_t expectedRevision,
    bool active)
{
    if (!database_.transactionActive() ||
        !safeAccountPart(accountId, 128) ||
        expectedRevision == 0)
    {
        return HumanAccountRepositoryStatus::invalid;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE security_human_accounts "
        "SET active = ?, revision = revision + 1, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE account_id = ? AND revision = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return HumanAccountRepositoryStatus::storageError;
    }

    const bool bound =
        sqlite3_bind_int(statement, 1, active ? 1 : 0) == SQLITE_OK &&
        bindText(statement, 2, accountId) &&
        sqlite3_bind_int64(
            statement,
            3,
            static_cast<sqlite3_int64>(expectedRevision)) == SQLITE_OK;
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
    {
        return HumanAccountRepositoryStatus::storageError;
    }
    if (changed == 1)
    {
        return HumanAccountRepositoryStatus::ok;
    }
    return mutationMissStatus(*this, accountId, expectedRevision);
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

HumanAccountLookupResult
HumanAccountRepository::findByActorId(
    const std::string& actorId) const
{
    HumanAccountLookupResult result;
    if (actorId.empty())
    {
        result.status = HumanAccountRepositoryStatus::invalid;
        return result;
    }

    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string(AccountSelect) +
        "WHERE account.actor_id = ?;";

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

    if (!bindText(statement, 1, actorId))
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


std::optional<std::size_t>
HumanAccountRepository::countUsableAdministratorsExcludingActor(
    const std::string& excludedActorId) const
{
    if (!excludedActorId.empty() &&
        !safeAccountPart(excludedActorId, 128))
    {
        return std::nullopt;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT COUNT(DISTINCT account.account_id) "
        "FROM security_human_accounts AS account "
        "JOIN security_actors AS actor "
        "ON actor.actor_id = account.actor_id "
        "JOIN security_actor_permission_grants AS grant_record "
        "ON grant_record.actor_id = actor.actor_id "
        "WHERE account.active <> 0 "
        "AND actor.actor_type = 'user' "
        "AND actor.active <> 0 "
        "AND actor.revoked_at = '' "
        "AND grant_record.permission = 'role.admin' "
        "AND grant_record.backend_id = '*' "
        "AND grant_record.active <> 0 "
        "AND grant_record.revoked_at = '' "
        "AND (?1 = '' OR actor.actor_id <> ?1) "
        "AND EXISTS ("
        "SELECT 1 "
        "FROM security_credentials AS credential "
        "JOIN security_basic_credential_verifiers AS verifier "
        "ON verifier.credential_id = credential.credential_id "
        "WHERE credential.actor_id = actor.actor_id "
        "AND credential.credential_type = 'human-password' "
        "AND credential.active <> 0 "
        "AND credential.revoked_at = '' "
        "AND (credential.expires_at = '' OR "
        "credential.expires_at > CURRENT_TIMESTAMP)"
        ");";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    if (!bindText(statement, 1, excludedActorId))
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::optional<std::size_t> result;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        const sqlite3_int64 count =
            sqlite3_column_int64(statement, 0);
        if (count >= 0)
        {
            result = static_cast<std::size_t>(count);
        }
    }

    sqlite3_finalize(statement);
    return result;
}
