#include "BrowserSessionCredentialRepository.h"

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

bool safeTimestamp(const std::string& value)
{
    if (value.empty() || value.size() > 64)
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
                character == '\n';
        });
}

bool safeHash(const std::string& value)
{
    return !value.empty() &&
        value.size() <= 1024 &&
        BrowserSessionCredentialRepository::supportsSecretHash(value) &&
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

bool validRegistration(
    const BrowserSessionCredentialRegistration& registration)
{
    return safeIdentifier(registration.tokenId) &&
        safeIdentifier(registration.sessionId) &&
        safeIdentifier(registration.actorId) &&
        safeIdentifier(registration.deviceId) &&
        safeIdentifier(registration.credentialId) &&
        safeIdentifier(registration.issuedFromCredentialId) &&
        safeHash(registration.sessionSecretHash) &&
        safeHash(registration.csrfSecretHash) &&
        safeTimestamp(registration.expiresAt);
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

std::optional<StoredBrowserSessionCredential> findOne(
    Database& database,
    const char* sql,
    const std::string& key)
{
    if (!safeIdentifier(key))
    {
        return std::nullopt;
    }

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    if (!bindText(statement, 1, key))
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::optional<StoredBrowserSessionCredential> result;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        StoredBrowserSessionCredential record;
        record.tokenId = columnText(statement, 0);
        record.sessionId = columnText(statement, 1);
        record.actorId = columnText(statement, 2);
        record.deviceId = columnText(statement, 3);
        record.credentialId = columnText(statement, 4);
        record.issuedFromCredentialId = columnText(statement, 5);
        record.sessionSecretHash = columnText(statement, 6);
        record.csrfSecretHash = columnText(statement, 7);
        record.expiresAt = columnText(statement, 8);
        record.active = sqlite3_column_int(statement, 9) != 0;
        record.expired = sqlite3_column_int(statement, 10) != 0;
        record.revoked = sqlite3_column_int(statement, 11) != 0;
        result = record;
    }

    sqlite3_finalize(statement);
    return result;
}

bool executeUpdate(
    Database& database,
    const char* sql,
    const std::string& key)
{
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound = bindText(statement, 1, key);
    const int result = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int changed = sqlite3_changes(database.handle());
    sqlite3_finalize(statement);
    return result == SQLITE_DONE && changed == 1;
}
}

BrowserSessionCredentialRepository::BrowserSessionCredentialRepository(
    Database& database)
    : database_(database)
{
}

bool BrowserSessionCredentialRepository::ensureSchema()
{
    return database_.execute(
               "CREATE TABLE IF NOT EXISTS security_browser_session_credentials ("
               "token_id TEXT PRIMARY KEY,"
               "session_id TEXT NOT NULL UNIQUE,"
               "actor_id TEXT NOT NULL,"
               "device_id TEXT NOT NULL,"
               "credential_id TEXT NOT NULL UNIQUE,"
               "issued_from_credential_id TEXT NOT NULL,"
               "session_secret_hash TEXT NOT NULL,"
               "csrf_secret_hash TEXT NOT NULL,"
               "active INTEGER NOT NULL DEFAULT 1,"
               "expires_at TEXT NOT NULL,"
               "revoked_at TEXT NOT NULL DEFAULT '',"
               "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id),"
               "FOREIGN KEY(device_id) REFERENCES security_devices(device_id),"
               "FOREIGN KEY(session_id) REFERENCES security_sessions(session_id),"
               "FOREIGN KEY(credential_id) REFERENCES security_credentials(credential_id),"
               "FOREIGN KEY(issued_from_credential_id) REFERENCES security_credentials(credential_id)"
               ");") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS idx_security_browser_sessions_actor "
               "ON security_browser_session_credentials(actor_id, session_id);") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS idx_security_browser_sessions_expiry "
               "ON security_browser_session_credentials(active, expires_at);");
}

bool BrowserSessionCredentialRepository::insert(
    const BrowserSessionCredentialRegistration& registration)
{
    if (!validRegistration(registration))
    {
        return false;
    }

    const char* sql =
        "INSERT INTO security_browser_session_credentials ("
        "token_id, session_id, actor_id, device_id, credential_id, "
        "issued_from_credential_id, session_secret_hash, csrf_secret_hash, "
        "expires_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
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
        bindText(statement, 1, registration.tokenId) &&
        bindText(statement, 2, registration.sessionId) &&
        bindText(statement, 3, registration.actorId) &&
        bindText(statement, 4, registration.deviceId) &&
        bindText(statement, 5, registration.credentialId) &&
        bindText(statement, 6, registration.issuedFromCredentialId) &&
        bindText(statement, 7, registration.sessionSecretHash) &&
        bindText(statement, 8, registration.csrfSecretHash) &&
        bindText(statement, 9, registration.expiresAt);
    const int result = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}

std::optional<StoredBrowserSessionCredential>
BrowserSessionCredentialRepository::findByTokenId(
    const std::string& tokenId) const
{
    return findOne(
        database_,
        "SELECT token_id, session_id, actor_id, device_id, credential_id, "
        "issued_from_credential_id, session_secret_hash, csrf_secret_hash, "
        "expires_at, active, "
        "(expires_at <= CURRENT_TIMESTAMP), revoked_at <> '' "
        "FROM security_browser_session_credentials WHERE token_id = ?;",
        tokenId);
}

std::optional<StoredBrowserSessionCredential>
BrowserSessionCredentialRepository::findBySessionId(
    const std::string& sessionId) const
{
    return findOne(
        database_,
        "SELECT token_id, session_id, actor_id, device_id, credential_id, "
        "issued_from_credential_id, session_secret_hash, csrf_secret_hash, "
        "expires_at, active, "
        "(expires_at <= CURRENT_TIMESTAMP), revoked_at <> '' "
        "FROM security_browser_session_credentials WHERE session_id = ?;",
        sessionId);
}

bool BrowserSessionCredentialRepository::revokeBySessionId(
    const std::string& sessionId)
{
    if (!safeIdentifier(sessionId))
    {
        return false;
    }

    return executeUpdate(
        database_,
        "UPDATE security_browser_session_credentials "
        "SET active = 0, revoked_at = CURRENT_TIMESTAMP, "
        "updated_at = CURRENT_TIMESTAMP WHERE session_id = ?;",
        sessionId);
}

bool BrowserSessionCredentialRepository::setExpiry(
    const std::string& sessionId,
    const std::string& expiresAt)
{
    if (!safeIdentifier(sessionId) || !safeTimestamp(expiresAt))
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE security_browser_session_credentials "
        "SET expires_at = ?, updated_at = CURRENT_TIMESTAMP "
        "WHERE session_id = ?;";

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
        bindText(statement, 1, expiresAt) &&
        bindText(statement, 2, sessionId);
    const int result = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    sqlite3_finalize(statement);
    return result == SQLITE_DONE && changed == 1;
}

bool BrowserSessionCredentialRepository::supportsSecretHash(
    const std::string& secretHash)
{
    return secretHash.rfind("$y$", 0) == 0 ||
        secretHash.rfind("$6$", 0) == 0;
}
