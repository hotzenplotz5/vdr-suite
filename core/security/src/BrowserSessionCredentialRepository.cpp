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

std::optional<StoredBrowserSessionCredential> findOne(
    Database& database,
    const char* sql,
    const std::string& key,
    int idleTimeoutSeconds = 0,
    bool bindIdleTimeout = false)
{
    if (!safeIdentifier(key) || idleTimeoutSeconds < 0)
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

    if (!bindText(statement, 1, key) ||
        (bindIdleTimeout &&
         sqlite3_bind_int(statement, 2, idleTimeoutSeconds) != SQLITE_OK))
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
        record.lastSeenAt = columnText(statement, 9);
        record.active = sqlite3_column_int(statement, 10) != 0;
        record.expired = sqlite3_column_int(statement, 11) != 0;
        record.idleExpired = sqlite3_column_int(statement, 12) != 0;
        record.revoked = sqlite3_column_int(statement, 13) != 0;
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
    if (!database_.execute(
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
            "last_seen_at TEXT NOT NULL DEFAULT '',"
            "revoked_at TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id),"
            "FOREIGN KEY(device_id) REFERENCES security_devices(device_id),"
            "FOREIGN KEY(session_id) REFERENCES security_sessions(session_id),"
            "FOREIGN KEY(credential_id) REFERENCES security_credentials(credential_id),"
            "FOREIGN KEY(issued_from_credential_id) REFERENCES security_credentials(credential_id)"
            ");"))
    {
        return false;
    }

    if (!columnExists(
            database_,
            "security_browser_session_credentials",
            "last_seen_at") &&
        !database_.execute(
            "ALTER TABLE security_browser_session_credentials "
            "ADD COLUMN last_seen_at TEXT NOT NULL DEFAULT '';"))
    {
        return false;
    }

    return database_.execute(
               "UPDATE security_browser_session_credentials "
               "SET last_seen_at = created_at WHERE last_seen_at = '';") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS idx_security_browser_sessions_actor "
               "ON security_browser_session_credentials(actor_id, session_id);") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS idx_security_browser_sessions_expiry "
               "ON security_browser_session_credentials(active, expires_at);") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS idx_security_browser_sessions_idle "
               "ON security_browser_session_credentials(active, last_seen_at);");
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
        "expires_at, last_seen_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP);";
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
        "expires_at, last_seen_at, active, "
        "(expires_at <= CURRENT_TIMESTAMP), 0, revoked_at <> '' "
        "FROM security_browser_session_credentials WHERE token_id = ?;",
        tokenId);
}

std::optional<StoredBrowserSessionCredential>
BrowserSessionCredentialRepository::findResolvedByTokenId(
    const std::string& tokenId,
    int idleTimeoutSeconds) const
{
    return findOne(
        database_,
        "SELECT browser.token_id, browser.session_id, browser.actor_id, "
        "browser.device_id, browser.credential_id, "
        "browser.issued_from_credential_id, browser.session_secret_hash, "
        "browser.csrf_secret_hash, browser.expires_at, "
        "browser.last_seen_at, "
        "(browser.active <> 0 AND "
        " issuing.credential_id IS NOT NULL AND "
        " issuing.actor_id = browser.actor_id AND "
        " issuing.active <> 0 AND issuing.revoked_at = ''), "
        "((browser.expires_at <= CURRENT_TIMESTAMP) OR "
        " (issuing.credential_id IS NOT NULL AND "
        "  issuing.actor_id = browser.actor_id AND "
        "  issuing.expires_at <> '' AND "
        "  issuing.expires_at <= CURRENT_TIMESTAMP)), "
        "(?2 > 0 AND browser.last_seen_at <= "
        " datetime(CURRENT_TIMESTAMP, '-' || ?2 || ' seconds')), "
        "((browser.revoked_at <> '') OR "
        " issuing.credential_id IS NULL OR "
        " issuing.actor_id <> browser.actor_id OR "
        " issuing.active = 0 OR issuing.revoked_at <> '') "
        "FROM security_browser_session_credentials AS browser "
        "LEFT JOIN security_credentials AS issuing "
        "ON issuing.credential_id = browser.issued_from_credential_id "
        "WHERE browser.token_id = ?1;",
        tokenId,
        idleTimeoutSeconds,
        true);
}

std::optional<StoredBrowserSessionCredential>
BrowserSessionCredentialRepository::findBySessionId(
    const std::string& sessionId) const
{
    return findOne(
        database_,
        "SELECT token_id, session_id, actor_id, device_id, credential_id, "
        "issued_from_credential_id, session_secret_hash, csrf_secret_hash, "
        "expires_at, last_seen_at, active, "
        "(expires_at <= CURRENT_TIMESTAMP), 0, revoked_at <> '' "
        "FROM security_browser_session_credentials WHERE session_id = ?;",
        sessionId);
}

std::optional<std::size_t>
BrowserSessionCredentialRepository::countEffectiveActiveByActorId(
    const std::string& actorId,
    int idleTimeoutSeconds) const
{
    if (!safeIdentifier(actorId) || idleTimeoutSeconds < 0)
    {
        return std::nullopt;
    }

    const char* sql =
        "SELECT COUNT(*) "
        "FROM security_browser_session_credentials AS browser "
        "JOIN security_actors AS actor "
        "ON actor.actor_id = browser.actor_id "
        "JOIN security_devices AS device "
        "ON device.device_id = browser.device_id "
        "AND device.actor_id = browser.actor_id "
        "JOIN security_sessions AS session "
        "ON session.session_id = browser.session_id "
        "AND session.actor_id = browser.actor_id "
        "AND session.device_id = browser.device_id "
        "JOIN security_credentials AS browser_credential "
        "ON browser_credential.credential_id = browser.credential_id "
        "AND browser_credential.actor_id = browser.actor_id "
        "JOIN security_credentials AS issuing_credential "
        "ON issuing_credential.credential_id = "
        "browser.issued_from_credential_id "
        "AND issuing_credential.actor_id = browser.actor_id "
        "WHERE browser.actor_id = ?1 "
        "AND browser.active <> 0 "
        "AND browser.revoked_at = '' "
        "AND browser.expires_at > CURRENT_TIMESTAMP "
        "AND (?2 = 0 OR browser.last_seen_at > "
        "datetime(CURRENT_TIMESTAMP, '-' || ?2 || ' seconds')) "
        "AND actor.active <> 0 "
        "AND actor.revoked_at = '' "
        "AND device.active <> 0 "
        "AND device.revoked_at = '' "
        "AND session.active <> 0 "
        "AND session.revoked_at = '' "
        "AND (session.expires_at = '' OR "
        "session.expires_at > CURRENT_TIMESTAMP) "
        "AND browser_credential.active <> 0 "
        "AND browser_credential.revoked_at = '' "
        "AND (browser_credential.expires_at = '' OR "
        "browser_credential.expires_at > CURRENT_TIMESTAMP) "
        "AND issuing_credential.active <> 0 "
        "AND issuing_credential.revoked_at = '' "
        "AND (issuing_credential.expires_at = '' OR "
        "issuing_credential.expires_at > CURRENT_TIMESTAMP);";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    if (!bindText(statement, 1, actorId) ||
        sqlite3_bind_int(statement, 2, idleTimeoutSeconds) != SQLITE_OK)
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::optional<std::size_t> result;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        const sqlite3_int64 count = sqlite3_column_int64(statement, 0);
        if (count >= 0)
        {
            result = static_cast<std::size_t>(count);
        }
    }

    sqlite3_finalize(statement);
    return result;
}

std::optional<bool>
BrowserSessionCredentialRepository::touchLastSeenIfDue(
    const std::string& tokenId,
    int minimumIntervalSeconds) const
{
    if (!safeIdentifier(tokenId) || minimumIntervalSeconds <= 0)
    {
        return std::nullopt;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE security_browser_session_credentials "
        "SET last_seen_at = CURRENT_TIMESTAMP, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE token_id = ?1 AND last_seen_at <= "
        "datetime(CURRENT_TIMESTAMP, '-' || ?2 || ' seconds');";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    const bool bound =
        bindText(statement, 1, tokenId) &&
        sqlite3_bind_int(
            statement,
            2,
            minimumIntervalSeconds) == SQLITE_OK;
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
    {
        return std::nullopt;
    }
    return changed == 1;
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
