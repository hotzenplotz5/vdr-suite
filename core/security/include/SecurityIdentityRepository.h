#pragma once

#include "Database.h"
#include "SecurityIdentity.h"

#include <sqlite3.h>

#include <optional>
#include <string>

struct StoredActorIdentity
{
    std::string actorId;
    ActorType type = ActorType::Anonymous;
    std::string displayName;
    bool active = false;
    bool revoked = false;
};

struct StoredDeviceIdentity
{
    std::string deviceId;
    std::string actorId;
    std::string displayName;
    bool active = false;
    bool revoked = false;
};

struct StoredSessionIdentity
{
    std::string sessionId;
    std::string actorId;
    std::string deviceId;
    bool active = false;
    bool expired = false;
    bool revoked = false;
};

struct StoredCredentialIdentity
{
    std::string credentialId;
    std::string actorId;
    std::string credentialType;
    bool active = false;
    bool expired = false;
    bool revoked = false;
};

class SecurityIdentityRepository
{
public:
    explicit SecurityIdentityRepository(Database& database)
        : database_(database)
    {
    }

    bool ensureSchema()
    {
        return database_.execute(
                   "CREATE TABLE IF NOT EXISTS security_actors ("
                   "actor_id TEXT PRIMARY KEY,"
                   "actor_type TEXT NOT NULL,"
                   "display_name TEXT NOT NULL,"
                   "active INTEGER NOT NULL DEFAULT 1,"
                   "revoked_at TEXT NOT NULL DEFAULT '',"
                   "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                   "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                   ");") &&
            database_.execute(
                   "CREATE TABLE IF NOT EXISTS security_devices ("
                   "device_id TEXT PRIMARY KEY,"
                   "actor_id TEXT NOT NULL,"
                   "display_name TEXT NOT NULL,"
                   "active INTEGER NOT NULL DEFAULT 1,"
                   "revoked_at TEXT NOT NULL DEFAULT '',"
                   "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                   "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                   "FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id)"
                   ");") &&
            database_.execute(
                   "CREATE INDEX IF NOT EXISTS idx_security_devices_actor "
                   "ON security_devices(actor_id, device_id);") &&
            database_.execute(
                   "CREATE TABLE IF NOT EXISTS security_sessions ("
                   "session_id TEXT PRIMARY KEY,"
                   "actor_id TEXT NOT NULL,"
                   "device_id TEXT NOT NULL,"
                   "active INTEGER NOT NULL DEFAULT 1,"
                   "expires_at TEXT NOT NULL DEFAULT '',"
                   "revoked_at TEXT NOT NULL DEFAULT '',"
                   "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                   "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                   "FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id),"
                   "FOREIGN KEY(device_id) REFERENCES security_devices(device_id)"
                   ");") &&
            database_.execute(
                   "CREATE INDEX IF NOT EXISTS idx_security_sessions_actor "
                   "ON security_sessions(actor_id, session_id);") &&
            database_.execute(
                   "CREATE TABLE IF NOT EXISTS security_credentials ("
                   "credential_id TEXT PRIMARY KEY,"
                   "actor_id TEXT NOT NULL,"
                   "credential_type TEXT NOT NULL,"
                   "active INTEGER NOT NULL DEFAULT 1,"
                   "expires_at TEXT NOT NULL DEFAULT '',"
                   "revoked_at TEXT NOT NULL DEFAULT '',"
                   "rotated_from_credential_id TEXT NOT NULL DEFAULT '',"
                   "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                   "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                   "FOREIGN KEY(actor_id) REFERENCES security_actors(actor_id)"
                   ");") &&
            database_.execute(
                   "CREATE INDEX IF NOT EXISTS idx_security_credentials_actor "
                   "ON security_credentials(actor_id, credential_id);");
    }

    bool ensureCompatibilityIdentity(
        const std::string& actorId,
        ActorType actorType,
        const std::string& actorDisplayName,
        const std::string& deviceId,
        const std::string& sessionId,
        const std::string& credentialId)
    {
        if (actorId.empty() || deviceId.empty() ||
            sessionId.empty() || credentialId.empty())
        {
            return false;
        }

        return insertActorIfMissing(
                   actorId,
                   actorTypeName(actorType),
                   actorDisplayName) &&
            insertDeviceIfMissing(
                   deviceId,
                   actorId,
                   "Legacy browser") &&
            insertSessionIfMissing(
                   sessionId,
                   actorId,
                   deviceId) &&
            insertCredentialIfMissing(
                   credentialId,
                   actorId,
                   "legacy-basic");
    }

    std::optional<StoredActorIdentity> findActor(
        const std::string& actorId) const
    {
        sqlite3_stmt* statement = nullptr;
        const char* sql =
            "SELECT actor_id, actor_type, display_name, active, "
            "revoked_at <> '' FROM security_actors WHERE actor_id = ?;";

        if (!prepareAndBind(statement, sql, actorId))
        {
            return std::nullopt;
        }

        std::optional<StoredActorIdentity> result;
        if (sqlite3_step(statement) == SQLITE_ROW)
        {
            StoredActorIdentity actor;
            actor.actorId = columnText(statement, 0);
            actor.type = actorTypeFromName(columnText(statement, 1));
            actor.displayName = columnText(statement, 2);
            actor.active = sqlite3_column_int(statement, 3) != 0;
            actor.revoked = sqlite3_column_int(statement, 4) != 0;
            result = actor;
        }
        sqlite3_finalize(statement);
        return result;
    }

    std::optional<StoredDeviceIdentity> findDevice(
        const std::string& deviceId) const
    {
        sqlite3_stmt* statement = nullptr;
        const char* sql =
            "SELECT device_id, actor_id, display_name, active, "
            "revoked_at <> '' FROM security_devices WHERE device_id = ?;";

        if (!prepareAndBind(statement, sql, deviceId))
        {
            return std::nullopt;
        }

        std::optional<StoredDeviceIdentity> result;
        if (sqlite3_step(statement) == SQLITE_ROW)
        {
            StoredDeviceIdentity device;
            device.deviceId = columnText(statement, 0);
            device.actorId = columnText(statement, 1);
            device.displayName = columnText(statement, 2);
            device.active = sqlite3_column_int(statement, 3) != 0;
            device.revoked = sqlite3_column_int(statement, 4) != 0;
            result = device;
        }
        sqlite3_finalize(statement);
        return result;
    }

    std::optional<StoredSessionIdentity> findSession(
        const std::string& sessionId) const
    {
        sqlite3_stmt* statement = nullptr;
        const char* sql =
            "SELECT session_id, actor_id, device_id, active, "
            "(expires_at <> '' AND expires_at <= CURRENT_TIMESTAMP), "
            "revoked_at <> '' "
            "FROM security_sessions WHERE session_id = ?;";

        if (!prepareAndBind(statement, sql, sessionId))
        {
            return std::nullopt;
        }

        std::optional<StoredSessionIdentity> result;
        if (sqlite3_step(statement) == SQLITE_ROW)
        {
            StoredSessionIdentity session;
            session.sessionId = columnText(statement, 0);
            session.actorId = columnText(statement, 1);
            session.deviceId = columnText(statement, 2);
            session.active = sqlite3_column_int(statement, 3) != 0;
            session.expired = sqlite3_column_int(statement, 4) != 0;
            session.revoked = sqlite3_column_int(statement, 5) != 0;
            result = session;
        }
        sqlite3_finalize(statement);
        return result;
    }

    std::optional<StoredCredentialIdentity> findCredential(
        const std::string& credentialId) const
    {
        sqlite3_stmt* statement = nullptr;
        const char* sql =
            "SELECT credential_id, actor_id, credential_type, active, "
            "(expires_at <> '' AND expires_at <= CURRENT_TIMESTAMP), "
            "revoked_at <> '' "
            "FROM security_credentials WHERE credential_id = ?;";

        if (!prepareAndBind(statement, sql, credentialId))
        {
            return std::nullopt;
        }

        std::optional<StoredCredentialIdentity> result;
        if (sqlite3_step(statement) == SQLITE_ROW)
        {
            StoredCredentialIdentity credential;
            credential.credentialId = columnText(statement, 0);
            credential.actorId = columnText(statement, 1);
            credential.credentialType = columnText(statement, 2);
            credential.active = sqlite3_column_int(statement, 3) != 0;
            credential.expired = sqlite3_column_int(statement, 4) != 0;
            credential.revoked = sqlite3_column_int(statement, 5) != 0;
            result = credential;
        }
        sqlite3_finalize(statement);
        return result;
    }

    bool revokeActor(const std::string& actorId)
    {
        return markRevoked("security_actors", "actor_id", actorId);
    }

    bool revokeDevice(const std::string& deviceId)
    {
        return markRevoked("security_devices", "device_id", deviceId);
    }

    bool revokeSession(const std::string& sessionId)
    {
        return markRevoked("security_sessions", "session_id", sessionId);
    }

    bool revokeCredential(const std::string& credentialId)
    {
        return markRevoked(
            "security_credentials",
            "credential_id",
            credentialId);
    }

    bool setSessionExpiry(
        const std::string& sessionId,
        const std::string& expiresAt)
    {
        return updateText(
            "security_sessions",
            "session_id",
            sessionId,
            "expires_at",
            expiresAt);
    }

    bool setCredentialExpiry(
        const std::string& credentialId,
        const std::string& expiresAt)
    {
        return updateText(
            "security_credentials",
            "credential_id",
            credentialId,
            "expires_at",
            expiresAt);
    }

private:
    bool insertActorIfMissing(
        const std::string& actorId,
        const std::string& actorType,
        const std::string& displayName)
    {
        const char* sql =
            "INSERT OR IGNORE INTO security_actors "
            "(actor_id, actor_type, display_name) VALUES (?, ?, ?);";
        return executeThreeText(sql, actorId, actorType, displayName);
    }

    bool insertDeviceIfMissing(
        const std::string& deviceId,
        const std::string& actorId,
        const std::string& displayName)
    {
        const char* sql =
            "INSERT OR IGNORE INTO security_devices "
            "(device_id, actor_id, display_name) VALUES (?, ?, ?);";
        return executeThreeText(sql, deviceId, actorId, displayName);
    }

    bool insertSessionIfMissing(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& deviceId)
    {
        const char* sql =
            "INSERT OR IGNORE INTO security_sessions "
            "(session_id, actor_id, device_id) VALUES (?, ?, ?);";
        return executeThreeText(sql, sessionId, actorId, deviceId);
    }

    bool insertCredentialIfMissing(
        const std::string& credentialId,
        const std::string& actorId,
        const std::string& credentialType)
    {
        const char* sql =
            "INSERT OR IGNORE INTO security_credentials "
            "(credential_id, actor_id, credential_type) VALUES (?, ?, ?);";
        return executeThreeText(
            sql,
            credentialId,
            actorId,
            credentialType);
    }

    bool executeThreeText(
        const char* sql,
        const std::string& first,
        const std::string& second,
        const std::string& third)
    {
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
            bindText(statement, 1, first) &&
            bindText(statement, 2, second) &&
            bindText(statement, 3, third);
        const int result = bound
            ? sqlite3_step(statement)
            : SQLITE_ERROR;
        sqlite3_finalize(statement);
        return result == SQLITE_DONE;
    }

    bool markRevoked(
        const std::string& table,
        const std::string& keyColumn,
        const std::string& keyValue)
    {
        if (keyValue.empty())
        {
            return false;
        }

        const std::string sql =
            "UPDATE " + table +
            " SET active = 0, revoked_at = CURRENT_TIMESTAMP, "
            "updated_at = CURRENT_TIMESTAMP WHERE " + keyColumn + " = ?;";
        sqlite3_stmt* statement = nullptr;

        if (sqlite3_prepare_v2(
                database_.handle(),
                sql.c_str(),
                -1,
                &statement,
                nullptr) != SQLITE_OK)
        {
            return false;
        }

        const bool bound = bindText(statement, 1, keyValue);
        const int result = bound
            ? sqlite3_step(statement)
            : SQLITE_ERROR;
        const int changed = sqlite3_changes(database_.handle());
        sqlite3_finalize(statement);
        return result == SQLITE_DONE && changed == 1;
    }

    bool updateText(
        const std::string& table,
        const std::string& keyColumn,
        const std::string& keyValue,
        const std::string& valueColumn,
        const std::string& value)
    {
        if (keyValue.empty())
        {
            return false;
        }

        const std::string sql =
            "UPDATE " + table + " SET " + valueColumn +
            " = ?, updated_at = CURRENT_TIMESTAMP WHERE " +
            keyColumn + " = ?;";
        sqlite3_stmt* statement = nullptr;

        if (sqlite3_prepare_v2(
                database_.handle(),
                sql.c_str(),
                -1,
                &statement,
                nullptr) != SQLITE_OK)
        {
            return false;
        }

        const bool bound =
            bindText(statement, 1, value) &&
            bindText(statement, 2, keyValue);
        const int result = bound
            ? sqlite3_step(statement)
            : SQLITE_ERROR;
        const int changed = sqlite3_changes(database_.handle());
        sqlite3_finalize(statement);
        return result == SQLITE_DONE && changed == 1;
    }

    bool prepareAndBind(
        sqlite3_stmt*& statement,
        const char* sql,
        const std::string& value) const
    {
        if (sqlite3_prepare_v2(
                database_.handle(),
                sql,
                -1,
                &statement,
                nullptr) != SQLITE_OK)
        {
            return false;
        }

        if (!bindText(statement, 1, value))
        {
            sqlite3_finalize(statement);
            statement = nullptr;
            return false;
        }
        return true;
    }

    static bool bindText(
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

    static std::string columnText(
        sqlite3_stmt* statement,
        int column)
    {
        const unsigned char* text =
            sqlite3_column_text(statement, column);
        return text == nullptr
            ? std::string()
            : std::string(reinterpret_cast<const char*>(text));
    }

    Database& database_;
};
