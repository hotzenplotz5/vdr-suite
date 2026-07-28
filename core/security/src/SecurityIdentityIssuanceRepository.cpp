#include "SecurityIdentityRepository.h"

#include "Database.h"

#include <sqlite3.h>

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

bool insertSession(
    Database& database,
    const std::string& sessionId,
    const std::string& actorId,
    const std::string& deviceId,
    const std::string& expiresAt)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO security_sessions "
        "(session_id, actor_id, device_id, expires_at) "
        "VALUES (?, ?, ?, ?);";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound =
        bindText(statement, 1, sessionId) &&
        bindText(statement, 2, actorId) &&
        bindText(statement, 3, deviceId) &&
        bindText(statement, 4, expiresAt);
    const int result = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}

bool insertCredential(
    Database& database,
    const std::string& credentialId,
    const std::string& actorId,
    const std::string& credentialType,
    const std::string& expiresAt,
    const std::string& issuedFromCredentialId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO security_credentials "
        "(credential_id, actor_id, credential_type, expires_at, "
        "rotated_from_credential_id) VALUES (?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound =
        bindText(statement, 1, credentialId) &&
        bindText(statement, 2, actorId) &&
        bindText(statement, 3, credentialType) &&
        bindText(statement, 4, expiresAt) &&
        bindText(statement, 5, issuedFromCredentialId);
    const int result = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}
}

bool SecurityIdentityRepository::createSessionCredential(
    const std::string& sessionId,
    const std::string& actorId,
    const std::string& deviceId,
    const std::string& credentialId,
    const std::string& credentialType,
    const std::string& expiresAt,
    const std::string& issuedFromCredentialId)
{
    if (sessionId.empty() ||
        actorId.empty() ||
        deviceId.empty() ||
        credentialId.empty() ||
        credentialType.empty() ||
        expiresAt.empty() ||
        issuedFromCredentialId.empty())
    {
        return false;
    }

    return insertSession(
               database_,
               sessionId,
               actorId,
               deviceId,
               expiresAt) &&
        insertCredential(
               database_,
               credentialId,
               actorId,
               credentialType,
               expiresAt,
               issuedFromCredentialId);
}
