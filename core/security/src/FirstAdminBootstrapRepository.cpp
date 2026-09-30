#include "FirstAdminBootstrapRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

namespace
{
bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128)
    {
        return false;
    }

    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return std::isalnum(character) ||
                character == '-' ||
                character == '_';
        });
}

bool safeTimestamp(const std::string& value)
{
    if (value.size() != 19)
    {
        return false;
    }

    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const char character = value[index];
        if (index == 4 || index == 7)
        {
            if (character != '-') return false;
        }
        else if (index == 10)
        {
            if (character != ' ') return false;
        }
        else if (index == 13 || index == 16)
        {
            if (character != ':') return false;
        }
        else if (!std::isdigit(static_cast<unsigned char>(character)))
        {
            return false;
        }
    }

    return true;
}

bool safeVerifierHash(const std::string& value)
{
    return !value.empty() &&
        value.size() <= 1024 &&
        FirstAdminBootstrapRepository::supportsVerifierHash(value) &&
        std::none_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return character == '\0' ||
                    character == '\r' ||
                    character == '\n';
            });
}

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

class DatabaseTransaction
{
public:
    explicit DatabaseTransaction(Database& database)
        : database_(database),
          active_(database_.execute("BEGIN IMMEDIATE;"))
    {
    }

    ~DatabaseTransaction()
    {
        if (active_)
        {
            database_.execute("ROLLBACK;");
        }
    }

    bool active() const noexcept
    {
        return active_;
    }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;"))
        {
            return false;
        }
        active_ = false;
        return true;
    }

private:
    Database& database_;
    bool active_ = false;
};

std::optional<bool> hasEffectiveBootstrap(Database& database)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT 1 FROM security_first_admin_bootstrap_issuances "
        "WHERE consumed_at = '' "
        "AND invalidated_at = '' "
        "AND expires_at > CURRENT_TIMESTAMP "
        "LIMIT 1;";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    const int step = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (step == SQLITE_ROW)
    {
        return true;
    }
    if (step == SQLITE_DONE)
    {
        return false;
    }
    return std::nullopt;
}

std::optional<bool> timestampIsFuture(
    Database& database,
    const std::string& timestamp)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT (? > CURRENT_TIMESTAMP);";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    if (!bindText(statement, 1, timestamp))
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    const int step = sqlite3_step(statement);
    if (step != SQLITE_ROW)
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    const bool future = sqlite3_column_int(statement, 0) != 0;
    sqlite3_finalize(statement);
    return future;
}

FirstAdminBootstrapLookupResult readBootstrap(
    Database& database,
    const std::string& bootstrapId)
{
    FirstAdminBootstrapLookupResult result;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT bootstrap_id, verifier_hash, expires_at, "
        "consumed_at, invalidated_at, "
        "(expires_at <= CURRENT_TIMESTAMP) "
        "FROM security_first_admin_bootstrap_issuances "
        "WHERE bootstrap_id = ?;";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return result;
    }

    if (!bindText(statement, 1, bootstrapId))
    {
        sqlite3_finalize(statement);
        return result;
    }

    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        result.bootstrap.bootstrapId = columnText(statement, 0);
        result.bootstrap.verifierHash = columnText(statement, 1);
        result.bootstrap.expiresAt = columnText(statement, 2);
        result.bootstrap.consumedAt = columnText(statement, 3);
        result.bootstrap.invalidatedAt = columnText(statement, 4);
        result.bootstrap.expired = sqlite3_column_int(statement, 5) != 0;
        result.bootstrap.consumed =
            !result.bootstrap.consumedAt.empty();
        result.bootstrap.invalidated =
            !result.bootstrap.invalidatedAt.empty();

        if (result.bootstrap.invalidated)
        {
            result.status = FirstAdminBootstrapStatus::invalidated;
        }
        else if (result.bootstrap.consumed)
        {
            result.status = FirstAdminBootstrapStatus::consumed;
        }
        else if (result.bootstrap.expired)
        {
            result.status = FirstAdminBootstrapStatus::expired;
        }
        else
        {
            result.status = FirstAdminBootstrapStatus::ok;
        }
    }
    else if (step == SQLITE_DONE)
    {
        result.status = FirstAdminBootstrapStatus::notFound;
    }

    sqlite3_finalize(statement);
    return result;
}

FirstAdminBootstrapStatus updateTerminalState(
    Database& database,
    const std::string& bootstrapId,
    const char* columnName,
    bool requireUnexpired)
{
    std::string sql =
        std::string("UPDATE security_first_admin_bootstrap_issuances SET ") +
        columnName +
        " = CURRENT_TIMESTAMP, updated_at = CURRENT_TIMESTAMP "
        "WHERE bootstrap_id = ? "
        "AND consumed_at = '' "
        "AND invalidated_at = '' ";

    if (requireUnexpired)
    {
        sql += "AND expires_at > CURRENT_TIMESTAMP ";
    }
    sql += ";";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return FirstAdminBootstrapStatus::storageError;
    }

    const bool bound = bindText(statement, 1, bootstrapId);
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int changed = sqlite3_changes(database.handle());
    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
    {
        return FirstAdminBootstrapStatus::storageError;
    }
    if (changed == 1)
    {
        return FirstAdminBootstrapStatus::ok;
    }

    const FirstAdminBootstrapLookupResult current =
        readBootstrap(database, bootstrapId);
    return current.status == FirstAdminBootstrapStatus::ok
        ? FirstAdminBootstrapStatus::storageError
        : current.status;
}
}

FirstAdminBootstrapRepository::FirstAdminBootstrapRepository(
    Database& database)
    : database_(database)
{
}

bool FirstAdminBootstrapRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();

    return database_.execute(
               "CREATE TABLE IF NOT EXISTS "
               "security_first_admin_bootstrap_issuances ("
               "bootstrap_id TEXT PRIMARY KEY,"
               "verifier_hash TEXT NOT NULL,"
               "expires_at TEXT NOT NULL,"
               "consumed_at TEXT NOT NULL DEFAULT '',"
               "invalidated_at TEXT NOT NULL DEFAULT '',"
               "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "CHECK(NOT (consumed_at <> '' AND invalidated_at <> ''))"
               ");") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS "
               "idx_security_first_admin_bootstrap_lifecycle "
               "ON security_first_admin_bootstrap_issuances("
               "consumed_at, invalidated_at, expires_at"
               ");") &&
        database_.execute(
               "CREATE TRIGGER IF NOT EXISTS "
               "security_first_admin_bootstrap_single_effective_insert "
               "BEFORE INSERT ON security_first_admin_bootstrap_issuances "
               "WHEN NEW.consumed_at = '' "
               "AND NEW.invalidated_at = '' "
               "AND NEW.expires_at > CURRENT_TIMESTAMP "
               "AND EXISTS ("
               "SELECT 1 FROM security_first_admin_bootstrap_issuances "
               "WHERE consumed_at = '' "
               "AND invalidated_at = '' "
               "AND expires_at > CURRENT_TIMESTAMP"
               ") BEGIN "
               "SELECT RAISE(ABORT, "
               "'active first-admin bootstrap already exists'); "
               "END;");
}

FirstAdminClaimState FirstAdminBootstrapRepository::claimState() const
{
    auto lease = database_.acquireTransactionLease();

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT 1 "
        "FROM security_human_accounts AS account "
        "JOIN security_actors AS actor "
        "ON actor.actor_id = account.actor_id "
        "JOIN security_actor_permission_grants AS grant_record "
        "ON grant_record.actor_id = account.actor_id "
        "WHERE account.active <> 0 "
        "AND actor.actor_type = 'user' "
        "AND actor.active <> 0 "
        "AND actor.revoked_at = '' "
        "AND grant_record.active <> 0 "
        "AND grant_record.revoked_at = '' "
        "AND grant_record.backend_id = '*' "
        "AND (grant_record.permission = 'role.admin' "
        "OR grant_record.permission = '*') "
        "LIMIT 1;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return FirstAdminClaimState::unavailable;
    }

    const int step = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (step == SQLITE_ROW)
    {
        return FirstAdminClaimState::claimed;
    }
    if (step == SQLITE_DONE)
    {
        return FirstAdminClaimState::unclaimed;
    }
    return FirstAdminClaimState::unavailable;
}

FirstAdminBootstrapStatus
FirstAdminBootstrapRepository::registerBootstrap(
    const FirstAdminBootstrapRegistration& registration)
{
    if (!safeIdentifier(registration.bootstrapId) ||
        !safeVerifierHash(registration.verifierHash) ||
        !safeTimestamp(registration.expiresAt))
    {
        return FirstAdminBootstrapStatus::invalid;
    }

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        return FirstAdminBootstrapStatus::storageError;
    }

    const FirstAdminClaimState state = claimState();
    if (state == FirstAdminClaimState::unavailable)
    {
        return FirstAdminBootstrapStatus::storageError;
    }
    if (state == FirstAdminClaimState::claimed)
    {
        return FirstAdminBootstrapStatus::claimed;
    }

    const std::optional<bool> future =
        timestampIsFuture(database_, registration.expiresAt);
    if (!future.has_value())
    {
        return FirstAdminBootstrapStatus::storageError;
    }
    if (!*future)
    {
        return FirstAdminBootstrapStatus::expired;
    }

    const std::optional<bool> effective =
        hasEffectiveBootstrap(database_);
    if (!effective.has_value())
    {
        return FirstAdminBootstrapStatus::storageError;
    }
    if (*effective)
    {
        return FirstAdminBootstrapStatus::conflict;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO security_first_admin_bootstrap_issuances "
        "(bootstrap_id, verifier_hash, expires_at) "
        "VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return FirstAdminBootstrapStatus::storageError;
    }

    const bool bound =
        bindText(statement, 1, registration.bootstrapId) &&
        bindText(statement, 2, registration.verifierHash) &&
        bindText(statement, 3, registration.expiresAt);
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
    {
        return FirstAdminBootstrapStatus::storageError;
    }

    return transaction.commit()
        ? FirstAdminBootstrapStatus::ok
        : FirstAdminBootstrapStatus::storageError;
}

FirstAdminBootstrapLookupResult
FirstAdminBootstrapRepository::findById(
    const std::string& bootstrapId) const
{
    FirstAdminBootstrapLookupResult result;
    if (!safeIdentifier(bootstrapId))
    {
        result.status = FirstAdminBootstrapStatus::invalid;
        return result;
    }

    auto lease = database_.acquireTransactionLease();

    const FirstAdminClaimState state = claimState();
    if (state == FirstAdminClaimState::unavailable)
    {
        result.status = FirstAdminBootstrapStatus::storageError;
        return result;
    }
    if (state == FirstAdminClaimState::claimed)
    {
        result.status = FirstAdminBootstrapStatus::claimed;
        return result;
    }

    return readBootstrap(database_, bootstrapId);
}

FirstAdminBootstrapStatus
FirstAdminBootstrapRepository::consumeInActiveTransaction(
    const std::string& bootstrapId)
{
    if (!safeIdentifier(bootstrapId))
    {
        return FirstAdminBootstrapStatus::invalid;
    }
    if (!database_.transactionActive())
    {
        return FirstAdminBootstrapStatus::transactionRequired;
    }

    const FirstAdminClaimState state = claimState();
    if (state == FirstAdminClaimState::unavailable)
    {
        return FirstAdminBootstrapStatus::storageError;
    }
    if (state == FirstAdminClaimState::claimed)
    {
        return FirstAdminBootstrapStatus::claimed;
    }

    return updateTerminalState(
        database_,
        bootstrapId,
        "consumed_at",
        true);
}

FirstAdminBootstrapStatus
FirstAdminBootstrapRepository::invalidateInActiveTransaction(
    const std::string& bootstrapId)
{
    if (!safeIdentifier(bootstrapId))
    {
        return FirstAdminBootstrapStatus::invalid;
    }
    if (!database_.transactionActive())
    {
        return FirstAdminBootstrapStatus::transactionRequired;
    }

    const FirstAdminClaimState state = claimState();
    if (state == FirstAdminClaimState::unavailable)
    {
        return FirstAdminBootstrapStatus::storageError;
    }
    if (state == FirstAdminClaimState::claimed)
    {
        return FirstAdminBootstrapStatus::claimed;
    }

    return updateTerminalState(
        database_,
        bootstrapId,
        "invalidated_at",
        false);
}

bool FirstAdminBootstrapRepository::supportsVerifierHash(
    const std::string& verifierHash)
{
    return verifierHash.rfind("$y$", 0) == 0 ||
        verifierHash.rfind("$6$", 0) == 0;
}
