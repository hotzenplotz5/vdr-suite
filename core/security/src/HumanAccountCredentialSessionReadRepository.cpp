#include "HumanAccountCredentialSessionReadRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <utility>

namespace
{
bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128)
        return false;

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

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
               statement, index, value.c_str(), -1, SQLITE_TRANSIENT) ==
        SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* text = sqlite3_column_text(statement, column);
    return text == nullptr
        ? std::string()
        : std::string(reinterpret_cast<const char*>(text));
}
}

HumanAccountCredentialSessionReadRepository::
HumanAccountCredentialSessionReadRepository(Database& database)
    : database_(database)
{
}

std::optional<std::vector<HumanAccountCredentialMetadata>>
HumanAccountCredentialSessionReadRepository::
listCredentialsByActorId(const std::string& actorId) const
{
    if (!safeIdentifier(actorId))
        return std::nullopt;

    const char* sql =
        "SELECT credential_id, credential_type, active, "
        "(expires_at <> '' AND expires_at <= CURRENT_TIMESTAMP), "
        "revoked_at <> '', expires_at, created_at "
        "FROM security_credentials "
        "WHERE actor_id = ? "
        "AND credential_type <> 'browser-session' "
        "ORDER BY credential_id ASC;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return std::nullopt;
    if (!bindText(statement, 1, actorId))
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::vector<HumanAccountCredentialMetadata> result;
    for (;;)
    {
        const int step = sqlite3_step(statement);
        if (step == SQLITE_DONE)
            break;
        if (step != SQLITE_ROW)
        {
            sqlite3_finalize(statement);
            return std::nullopt;
        }

        HumanAccountCredentialMetadata credential;
        credential.credentialId = columnText(statement, 0);
        credential.credentialType = columnText(statement, 1);
        credential.active = sqlite3_column_int(statement, 2) != 0;
        credential.expired = sqlite3_column_int(statement, 3) != 0;
        credential.revoked = sqlite3_column_int(statement, 4) != 0;
        credential.expiresAt = columnText(statement, 5);
        credential.createdAt = columnText(statement, 6);
        result.push_back(std::move(credential));
    }

    sqlite3_finalize(statement);
    return result;
}

std::optional<std::vector<HumanAccountSessionMetadata>>
HumanAccountCredentialSessionReadRepository::
listSessionsByActorId(const std::string& actorId) const
{
    if (!safeIdentifier(actorId))
        return std::nullopt;

    const char* sql =
        "SELECT browser.session_id, browser.device_id, "
        "browser.issued_from_credential_id, "
        "(browser.active <> 0 AND session.active <> 0 AND "
        " browser_credential.active <> 0 AND issuing_credential.active <> 0), "
        "((browser.expires_at <> '' AND browser.expires_at <= CURRENT_TIMESTAMP) OR "
        " (session.expires_at <> '' AND session.expires_at <= CURRENT_TIMESTAMP) OR "
        " (browser_credential.expires_at <> '' AND browser_credential.expires_at <= CURRENT_TIMESTAMP) OR "
        " (issuing_credential.expires_at <> '' AND issuing_credential.expires_at <= CURRENT_TIMESTAMP)), "
        "((browser.revoked_at <> '') OR (session.revoked_at <> '') OR "
        " (browser_credential.revoked_at <> '') OR (issuing_credential.revoked_at <> '')), "
        "browser.expires_at, browser.last_seen_at, browser.created_at "
        "FROM security_browser_session_credentials AS browser "
        "JOIN security_sessions AS session "
        "ON session.session_id = browser.session_id "
        "AND session.actor_id = browser.actor_id "
        "AND session.device_id = browser.device_id "
        "JOIN security_credentials AS browser_credential "
        "ON browser_credential.credential_id = browser.credential_id "
        "AND browser_credential.actor_id = browser.actor_id "
        "AND browser_credential.credential_type = 'browser-session' "
        "JOIN security_credentials AS issuing_credential "
        "ON issuing_credential.credential_id = browser.issued_from_credential_id "
        "AND issuing_credential.actor_id = browser.actor_id "
        "WHERE browser.actor_id = ? "
        "ORDER BY browser.session_id ASC;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return std::nullopt;
    if (!bindText(statement, 1, actorId))
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::vector<HumanAccountSessionMetadata> result;
    for (;;)
    {
        const int step = sqlite3_step(statement);
        if (step == SQLITE_DONE)
            break;
        if (step != SQLITE_ROW)
        {
            sqlite3_finalize(statement);
            return std::nullopt;
        }

        HumanAccountSessionMetadata session;
        session.sessionId = columnText(statement, 0);
        session.deviceId = columnText(statement, 1);
        session.issuedFromCredentialId = columnText(statement, 2);
        session.active = sqlite3_column_int(statement, 3) != 0;
        session.expired = sqlite3_column_int(statement, 4) != 0;
        session.revoked = sqlite3_column_int(statement, 5) != 0;
        session.expiresAt = columnText(statement, 6);
        session.lastSeenAt = columnText(statement, 7);
        session.createdAt = columnText(statement, 8);
        result.push_back(std::move(session));
    }

    sqlite3_finalize(statement);
    return result;
}
