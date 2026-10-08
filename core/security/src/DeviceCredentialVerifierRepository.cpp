#include "DeviceCredentialVerifierRepository.h"

#include "Database.h"

#include <sqlite3.h>

namespace
{
bool bindText(sqlite3_stmt* stmt, int index, const std::string& value)
{
    return sqlite3_bind_text(stmt, index, value.c_str(), -1,
                             SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string readText(sqlite3_stmt* stmt, int index)
{
    const unsigned char* value = sqlite3_column_text(stmt, index);
    return value == nullptr ? std::string() :
        std::string(reinterpret_cast<const char*>(value));
}
}

DeviceCredentialVerifierRepository::DeviceCredentialVerifierRepository(
    Database& database) : database_(database)
{
}

bool DeviceCredentialVerifierRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS security_device_credential_verifiers ("
        "credential_id TEXT PRIMARY KEY,"
        "device_id TEXT NOT NULL,"
        "verifier_hash TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY(credential_id) REFERENCES security_credentials(credential_id),"
        "FOREIGN KEY(device_id) REFERENCES security_devices(device_id)"
        ");");
}

bool DeviceCredentialVerifierRepository::insertInActiveTransaction(
    const std::string& credentialId, const std::string& deviceId,
    const std::string& verifierHash)
{
    if (!database_.transactionActive() || credentialId.empty() ||
        deviceId.empty() || verifierHash.size() < 40U ||
        verifierHash.size() > 1024U ||
        verifierHash.rfind("$6$", 0U) != 0U)
        return false;

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO security_device_credential_verifiers "
        "(credential_id, device_id, verifier_hash) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &stmt, nullptr)
        != SQLITE_OK)
        return false;
    const bool bound = bindText(stmt, 1, credentialId) &&
        bindText(stmt, 2, deviceId) && bindText(stmt, 3, verifierHash);
    const int step = bound ? sqlite3_step(stmt) : SQLITE_ERROR;
    sqlite3_finalize(stmt);
    return step == SQLITE_DONE;
}

std::optional<StoredDeviceCredentialVerifier>
DeviceCredentialVerifierRepository::findByCredentialId(
    const std::string& credentialId) const
{
    if (credentialId.empty())
        return std::nullopt;
    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(database_.handle(),
        "SELECT credential_id, device_id, verifier_hash "
        "FROM security_device_credential_verifiers "
        "WHERE credential_id = ?;", -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;
    if (!bindText(stmt, 1, credentialId))
    {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    std::optional<StoredDeviceCredentialVerifier> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = StoredDeviceCredentialVerifier{
            readText(stmt, 0), readText(stmt, 1), readText(stmt, 2)};
    sqlite3_finalize(stmt);
    return result;
}
