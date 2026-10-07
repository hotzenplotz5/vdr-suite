#include "DevicePairingRequestRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>

namespace
{
bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128U)
        return false;

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
    if (value.size() != 19U)
        return false;

    for (std::size_t index = 0U; index < value.size(); ++index)
    {
        const char character = value[index];
        if (index == 4U || index == 7U)
        {
            if (character != '-') return false;
        }
        else if (index == 10U)
        {
            if (character != ' ') return false;
        }
        else if (index == 13U || index == 16U)
        {
            if (character != ':') return false;
        }
        else if (!std::isdigit(
                     static_cast<unsigned char>(character)))
        {
            return false;
        }
    }
    return true;
}

bool safeText(
    const std::string& value,
    std::size_t maximumLength,
    bool allowEmpty)
{
    if ((!allowEmpty && value.empty()) ||
        value.size() > maximumLength)
    {
        return false;
    }

    return std::none_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return character == 0U ||
                character == '\r' ||
                character == '\n';
        });
}

bool safeSecretHash(const std::string& value)
{
    return !value.empty() &&
        value.size() <= 1024U &&
        DevicePairingRequestRepository::supportsSecretHash(value) &&
        std::none_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return character == 0U ||
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
    const unsigned char* text =
        sqlite3_column_text(statement, column);
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

bool validState(const std::string& state)
{
    return state == "pending" ||
        state == "approved" ||
        state == "rejected";
}

bool readStoredRequest(
    sqlite3_stmt* statement,
    StoredDevicePairingRequest& request)
{
    request.pairingRequestId = columnText(statement, 0);
    request.userCodeHash = columnText(statement, 1);
    request.pairingTokenHash = columnText(statement, 2);
    request.displayName = columnText(statement, 3);
    request.clientKind = columnText(statement, 4);
    request.appVersion = columnText(statement, 5);
    request.state = columnText(statement, 6);
    request.expiresAt = columnText(statement, 7);
    request.createdAt = columnText(statement, 8);

    const sqlite3_int64 revision =
        sqlite3_column_int64(statement, 9);
    request.decidedByActorId = columnText(statement, 10);
    request.decidedAt = columnText(statement, 11);
    request.expired = sqlite3_column_int(statement, 12) != 0;
    request.invalidated = sqlite3_column_int(statement, 13) != 0;

    if (revision <= 0 ||
        !validState(request.state) ||
        request.pairingRequestId.empty() ||
        request.userCodeHash.empty() ||
        request.pairingTokenHash.empty() ||
        request.displayName.empty() ||
        request.clientKind.empty() ||
        request.expiresAt.empty() ||
        request.createdAt.empty())
    {
        return false;
    }

    request.revision =
        static_cast<std::uint64_t>(revision);
    return true;
}

constexpr const char* RequestSelect =
    "SELECT pairing_request_id, user_code_hash, "
    "pairing_token_hash, display_name, client_kind, "
    "app_version, state, expires_at, created_at, revision, "
    "decided_by_actor_id, decided_at, "
    "(expires_at <= CURRENT_TIMESTAMP), "
    "(invalidated_at <> '') "
    "FROM security_device_pairing_requests ";

DevicePairingRequestRepositoryStatus statusForStored(
    const StoredDevicePairingRequest& request)
{
    if (request.invalidated)
        return DevicePairingRequestRepositoryStatus::invalidated;
    if (request.expired)
        return DevicePairingRequestRepositoryStatus::expired;
    return DevicePairingRequestRepositoryStatus::ok;
}
}

DevicePairingRequestRepository::DevicePairingRequestRepository(
    Database& database)
    : database_(database)
{
}

bool DevicePairingRequestRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();

    if (!database_.execute(
            "CREATE TABLE IF NOT EXISTS "
            "security_device_pairing_requests ("
            "pairing_request_id TEXT PRIMARY KEY,"
            "user_code_hash TEXT NOT NULL UNIQUE,"
            "pairing_token_hash TEXT NOT NULL UNIQUE,"
            "display_name TEXT NOT NULL,"
            "client_kind TEXT NOT NULL,"
            "app_version TEXT NOT NULL DEFAULT '',"
            "state TEXT NOT NULL DEFAULT 'pending',"
            "expires_at TEXT NOT NULL,"
            "invalidated_at TEXT NOT NULL DEFAULT '',"
            "revision INTEGER NOT NULL DEFAULT 1,"
            "decided_by_actor_id TEXT NOT NULL DEFAULT '',"
            "decided_at TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ");"))
    {
        return false;
    }

    if (!columnExists(
            database_,
            "security_device_pairing_requests",
            "revision") &&
        !database_.execute(
            "ALTER TABLE security_device_pairing_requests "
            "ADD COLUMN revision INTEGER NOT NULL DEFAULT 1;"))
    {
        return false;
    }

    if (!columnExists(
            database_,
            "security_device_pairing_requests",
            "decided_by_actor_id") &&
        !database_.execute(
            "ALTER TABLE security_device_pairing_requests "
            "ADD COLUMN decided_by_actor_id TEXT NOT NULL DEFAULT '';"))
    {
        return false;
    }

    if (!columnExists(
            database_,
            "security_device_pairing_requests",
            "decided_at") &&
        !database_.execute(
            "ALTER TABLE security_device_pairing_requests "
            "ADD COLUMN decided_at TEXT NOT NULL DEFAULT '';"))
    {
        return false;
    }

    return database_.execute(
               "UPDATE security_device_pairing_requests "
               "SET revision = 1 WHERE revision <= 0;") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS "
               "idx_security_device_pairing_requests_lifecycle "
               "ON security_device_pairing_requests("
               "state, invalidated_at, expires_at"
               ");");
}

DevicePairingRequestRepositoryStatus
DevicePairingRequestRepository::registerInActiveTransaction(
    const DevicePairingRequestRegistration& registration)
{
    if (!database_.transactionActive())
    {
        return DevicePairingRequestRepositoryStatus::
            transactionRequired;
    }

    if (!safeIdentifier(registration.pairingRequestId) ||
        !safeSecretHash(registration.userCodeHash) ||
        !safeSecretHash(registration.pairingTokenHash) ||
        !safeText(registration.displayName, 128U, false) ||
        !safeText(registration.clientKind, 64U, false) ||
        !safeText(registration.appVersion, 64U, true) ||
        !safeTimestamp(registration.expiresAt))
    {
        return DevicePairingRequestRepositoryStatus::invalid;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO security_device_pairing_requests ("
        "pairing_request_id, user_code_hash, pairing_token_hash, "
        "display_name, client_kind, app_version, expires_at, revision"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, 1);";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return DevicePairingRequestRepositoryStatus::storageError;
    }

    const bool bound =
        bindText(statement, 1, registration.pairingRequestId) &&
        bindText(statement, 2, registration.userCodeHash) &&
        bindText(statement, 3, registration.pairingTokenHash) &&
        bindText(statement, 4, registration.displayName) &&
        bindText(statement, 5, registration.clientKind) &&
        bindText(statement, 6, registration.appVersion) &&
        bindText(statement, 7, registration.expiresAt);
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int extended =
        sqlite3_extended_errcode(database_.handle());
    sqlite3_finalize(statement);

    if (step == SQLITE_DONE)
    {
        return DevicePairingRequestRepositoryStatus::ok;
    }
    if (extended == SQLITE_CONSTRAINT ||
        extended == SQLITE_CONSTRAINT_PRIMARYKEY ||
        extended == SQLITE_CONSTRAINT_UNIQUE)
    {
        return DevicePairingRequestRepositoryStatus::conflict;
    }
    return DevicePairingRequestRepositoryStatus::storageError;
}

DevicePairingRequestLookupResult
DevicePairingRequestRepository::findById(
    const std::string& pairingRequestId) const
{
    DevicePairingRequestLookupResult result;
    if (!safeIdentifier(pairingRequestId))
    {
        result.status =
            DevicePairingRequestRepositoryStatus::invalid;
        return result;
    }

    auto lease = database_.acquireTransactionLease();

    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string(RequestSelect) +
        "WHERE pairing_request_id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return result;
    }

    if (!bindText(statement, 1, pairingRequestId))
    {
        sqlite3_finalize(statement);
        return result;
    }

    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        if (!readStoredRequest(statement, result.request))
        {
            sqlite3_finalize(statement);
            return result;
        }
        result.status = statusForStored(result.request);
    }
    else if (step == SQLITE_DONE)
    {
        result.status =
            DevicePairingRequestRepositoryStatus::notFound;
    }

    sqlite3_finalize(statement);
    return result;
}

DevicePairingRequestListResult
DevicePairingRequestRepository::listPending(
    const std::string& afterPairingRequestId,
    std::size_t limit) const
{
    DevicePairingRequestListResult result;
    if ((!afterPairingRequestId.empty() &&
         !safeIdentifier(afterPairingRequestId)) ||
        limit == 0U ||
        limit > 100U)
    {
        result.status = DevicePairingRequestRepositoryStatus::invalid;
        return result;
    }

    auto lease = database_.acquireTransactionLease();

    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string(RequestSelect) +
        "WHERE state = 'pending' "
        "AND invalidated_at = '' "
        "AND expires_at > CURRENT_TIMESTAMP "
        "AND pairing_request_id > ? "
        "ORDER BY pairing_request_id ASC "
        "LIMIT ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return result;
    }

    const bool bound =
        bindText(statement, 1, afterPairingRequestId) &&
        sqlite3_bind_int64(
            statement,
            2,
            static_cast<sqlite3_int64>(limit + 1U)) == SQLITE_OK;
    if (!bound)
    {
        sqlite3_finalize(statement);
        return result;
    }

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        StoredDevicePairingRequest request;
        if (!readStoredRequest(statement, request) ||
            request.state != "pending" ||
            request.expired ||
            request.invalidated)
        {
            sqlite3_finalize(statement);
            result.requests.clear();
            return result;
        }
        result.requests.push_back(std::move(request));
    }
    sqlite3_finalize(statement);

    if (result.requests.size() > limit)
    {
        result.hasMore = true;
        result.requests.resize(limit);
    }
    result.status = DevicePairingRequestRepositoryStatus::ok;
    return result;
}

DevicePairingRequestRepositoryStatus
DevicePairingRequestRepository::decideInActiveTransaction(
    const std::string& pairingRequestId,
    std::uint64_t expectedRevision,
    const std::string& state,
    const std::string& decidedByActorId)
{
    if (!database_.transactionActive())
    {
        return DevicePairingRequestRepositoryStatus::
            transactionRequired;
    }

    if (!safeIdentifier(pairingRequestId) ||
        expectedRevision == 0U ||
        expectedRevision >
            static_cast<std::uint64_t>(
                std::numeric_limits<sqlite3_int64>::max()) ||
        (state != "approved" && state != "rejected") ||
        !safeText(decidedByActorId, 128U, false))
    {
        return DevicePairingRequestRepositoryStatus::invalid;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE security_device_pairing_requests "
        "SET state = ?, decided_by_actor_id = ?, "
        "decided_at = CURRENT_TIMESTAMP, "
        "revision = revision + 1, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE pairing_request_id = ? "
        "AND revision = ? "
        "AND state = 'pending' "
        "AND invalidated_at = '' "
        "AND expires_at > CURRENT_TIMESTAMP;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return DevicePairingRequestRepositoryStatus::storageError;
    }

    const bool bound =
        bindText(statement, 1, state) &&
        bindText(statement, 2, decidedByActorId) &&
        bindText(statement, 3, pairingRequestId) &&
        sqlite3_bind_int64(
            statement,
            4,
            static_cast<sqlite3_int64>(expectedRevision)) == SQLITE_OK;
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    sqlite3_finalize(statement);

    if (step != SQLITE_DONE)
        return DevicePairingRequestRepositoryStatus::storageError;
    if (changed == 1)
        return DevicePairingRequestRepositoryStatus::ok;

    const DevicePairingRequestLookupResult current =
        findById(pairingRequestId);
    if (current.status != DevicePairingRequestRepositoryStatus::ok)
        return current.status;
    if (current.request.revision != expectedRevision)
        return DevicePairingRequestRepositoryStatus::revisionConflict;
    if (current.request.state != "pending")
        return DevicePairingRequestRepositoryStatus::stateConflict;
    return DevicePairingRequestRepositoryStatus::storageError;
}

bool DevicePairingRequestRepository::supportsSecretHash(
    const std::string& secretHash)
{
    return secretHash.rfind("$6$", 0U) == 0U;
}
