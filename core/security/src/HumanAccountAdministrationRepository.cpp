#include "HumanAccountAdministrationRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace
{
bool safeIdentifierOrEmpty(const std::string& value)
{
    if (value.size() > 128)
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
}

HumanAccountAdministrationRepository::
HumanAccountAdministrationRepository(Database& database)
    : database_(database)
{
}

std::optional<std::size_t>
HumanAccountAdministrationRepository::
countUsableAdministratorsExcludingActor(
    const std::string& excludedActorId) const
{
    if (!safeIdentifierOrEmpty(excludedActorId))
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


std::optional<std::size_t>
HumanAccountAdministrationRepository::
countUsableAdministratorsExcludingCredential(
    const std::string& excludedCredentialId) const
{
    if (!safeIdentifierOrEmpty(excludedCredentialId))
        return std::nullopt;

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
        "credential.expires_at > CURRENT_TIMESTAMP) "
        "AND (?1 = '' OR credential.credential_id <> ?1)"
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

    if (!bindText(statement, 1, excludedCredentialId))
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
            result = static_cast<std::size_t>(count);
    }

    sqlite3_finalize(statement);
    return result;
}
