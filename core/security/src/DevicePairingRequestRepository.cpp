#include "DevicePairingRequestRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
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
}

DevicePairingRequestRepository::DevicePairingRequestRepository(
    Database& database)
    : database_(database)
{
}

bool DevicePairingRequestRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();

    return database_.execute(
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
               "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
               ");") &&
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
        "display_name, client_kind, app_version, expires_at"
        ") VALUES (?, ?, ?, ?, ?, ?, ?);";

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
    const char* sql =
        "SELECT pairing_request_id, user_code_hash, "
        "pairing_token_hash, display_name, client_kind, "
        "app_version, state, expires_at, created_at, "
        "(expires_at <= CURRENT_TIMESTAMP), "
        "(invalidated_at <> '') "
        "FROM security_device_pairing_requests "
        "WHERE pairing_request_id = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
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
        result.request.pairingRequestId =
            columnText(statement, 0);
        result.request.userCodeHash =
            columnText(statement, 1);
        result.request.pairingTokenHash =
            columnText(statement, 2);
        result.request.displayName =
            columnText(statement, 3);
        result.request.clientKind =
            columnText(statement, 4);
        result.request.appVersion =
            columnText(statement, 5);
        result.request.state =
            columnText(statement, 6);
        result.request.expiresAt =
            columnText(statement, 7);
        result.request.createdAt =
            columnText(statement, 8);
        result.request.expired =
            sqlite3_column_int(statement, 9) != 0;
        result.request.invalidated =
            sqlite3_column_int(statement, 10) != 0;

        if (result.request.invalidated)
        {
            result.status =
                DevicePairingRequestRepositoryStatus::invalidated;
        }
        else if (result.request.expired)
        {
            result.status =
                DevicePairingRequestRepositoryStatus::expired;
        }
        else
        {
            result.status =
                DevicePairingRequestRepositoryStatus::ok;
        }
    }
    else if (step == SQLITE_DONE)
    {
        result.status =
            DevicePairingRequestRepositoryStatus::notFound;
    }

    sqlite3_finalize(statement);
    return result;
}

bool DevicePairingRequestRepository::supportsSecretHash(
    const std::string& secretHash)
{
    return secretHash.rfind("$6$", 0U) == 0U;
}
