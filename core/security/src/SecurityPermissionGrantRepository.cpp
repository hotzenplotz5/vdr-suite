#include "SecurityPermissionGrantRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
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
    const unsigned char* value =
        sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string()
        : std::string(
            reinterpret_cast<const char*>(value));
}

bool safeGrantPart(
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
}

SecurityPermissionGrantRepository::
SecurityPermissionGrantRepository(Database& database)
    : database_(database)
{
}

bool SecurityPermissionGrantRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();

    return database_.execute(
               "CREATE TABLE IF NOT EXISTS "
               "security_actor_permission_grants ("
               "actor_id TEXT NOT NULL,"
               "permission TEXT NOT NULL,"
               "backend_id TEXT NOT NULL,"
               "active INTEGER NOT NULL DEFAULT 1,"
               "revoked_at TEXT NOT NULL DEFAULT '',"
               "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "PRIMARY KEY(actor_id, permission, backend_id),"
               "FOREIGN KEY(actor_id) "
               "REFERENCES security_actors(actor_id)"
               ");") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS "
               "idx_security_actor_permission_grants_active "
               "ON security_actor_permission_grants("
               "actor_id, active, permission, backend_id"
               ");");
}

SecurityPermissionGrantResolution
SecurityPermissionGrantRepository::findActiveGrantsForActor(
    const std::string& actorId) const
{
    SecurityPermissionGrantResolution resolution;

    if (!safeGrantPart(actorId, 128))
    {
        return resolution;
    }

    auto lease = database_.acquireTransactionLease();

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT permission, backend_id "
        "FROM security_actor_permission_grants "
        "WHERE actor_id = ? "
        "AND active = 1 "
        "AND revoked_at = '' "
        "ORDER BY permission, backend_id;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return resolution;
    }

    if (!bindText(statement, 1, actorId))
    {
        sqlite3_finalize(statement);
        return resolution;
    }

    int result = SQLITE_OK;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        PermissionGrant grant;
        grant.permission = columnText(statement, 0);
        grant.backendId = columnText(statement, 1);

        if (!safeGrantPart(grant.permission, 256) ||
            !safeGrantPart(grant.backendId, 256))
        {
            sqlite3_finalize(statement);
            resolution.grants.clear();
            return resolution;
        }

        resolution.grants.push_back(std::move(grant));
    }

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE)
    {
        resolution.grants.clear();
        return resolution;
    }

    resolution.available = true;
    return resolution;
}

bool SecurityPermissionGrantRepository::ensureGrant(
    const std::string& actorId,
    const std::string& permission,
    const std::string& backendId)
{
    if (!safeGrantPart(actorId, 128) ||
        !safeGrantPart(permission, 256) ||
        !safeGrantPart(backendId, 256))
    {
        return false;
    }

    auto lease = database_.acquireTransactionLease();

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO security_actor_permission_grants "
        "(actor_id, permission, backend_id) "
        "VALUES (?, ?, ?) "
        "ON CONFLICT(actor_id, permission, backend_id) "
        "DO UPDATE SET "
        "active = 1, "
        "revoked_at = '', "
        "updated_at = CURRENT_TIMESTAMP;";

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
        bindText(statement, 1, actorId) &&
        bindText(statement, 2, permission) &&
        bindText(statement, 3, backendId);

    const int result = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;

    sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}

bool SecurityPermissionGrantRepository::revokeGrant(
    const std::string& actorId,
    const std::string& permission,
    const std::string& backendId)
{
    if (!safeGrantPart(actorId, 128) ||
        !safeGrantPart(permission, 256) ||
        !safeGrantPart(backendId, 256))
    {
        return false;
    }

    auto lease = database_.acquireTransactionLease();

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE security_actor_permission_grants "
        "SET active = 0, "
        "revoked_at = CURRENT_TIMESTAMP, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE actor_id = ? "
        "AND permission = ? "
        "AND backend_id = ? "
        "AND active = 1;";

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
        bindText(statement, 1, actorId) &&
        bindText(statement, 2, permission) &&
        bindText(statement, 3, backendId);

    const int result = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());

    sqlite3_finalize(statement);
    return result == SQLITE_DONE && changed == 1;
}
