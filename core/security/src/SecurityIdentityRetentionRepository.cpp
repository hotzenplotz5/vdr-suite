#include "SecurityIdentityRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>

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
                character == '_' ||
                character == '.' ||
                character == ':';
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

bool SecurityIdentityRepository::deleteSessionIfUnreferenced(
    const std::string& sessionId,
    const std::string& actorId,
    const std::string& deviceId)
{
    if (!safeIdentifier(sessionId) ||
        !safeIdentifier(actorId) ||
        !safeIdentifier(deviceId))
    {
        return false;
    }

    const char* sql =
        "DELETE FROM security_sessions "
        "WHERE session_id = ?1 AND actor_id = ?2 AND device_id = ?3 "
        "AND NOT EXISTS ("
        "SELECT 1 FROM security_browser_session_credentials "
        "WHERE session_id = ?1);";
    sqlite3_stmt* statement = nullptr;
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
        bindText(statement, 1, sessionId) &&
        bindText(statement, 2, actorId) &&
        bindText(statement, 3, deviceId);
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int finalizeResult = sqlite3_finalize(statement);
    return step == SQLITE_DONE && finalizeResult == SQLITE_OK;
}

bool SecurityIdentityRepository::deleteBrowserSessionCredentialIfUnreferenced(
    const std::string& credentialId,
    const std::string& actorId)
{
    if (!safeIdentifier(credentialId) || !safeIdentifier(actorId))
    {
        return false;
    }

    const char* sql =
        "DELETE FROM security_credentials "
        "WHERE credential_id = ?1 AND actor_id = ?2 "
        "AND credential_type = 'browser-session' "
        "AND NOT EXISTS ("
        "SELECT 1 FROM security_browser_session_credentials "
        "WHERE credential_id = ?1);";
    sqlite3_stmt* statement = nullptr;
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
        bindText(statement, 1, credentialId) &&
        bindText(statement, 2, actorId);
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int finalizeResult = sqlite3_finalize(statement);
    return step == SQLITE_DONE && finalizeResult == SQLITE_OK;
}