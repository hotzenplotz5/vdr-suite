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
    // Canonical ownership and lifecycle must match at insert time.
    // This table is only a verifier binding, never another Identity Authority.
    const char* sql =
        "INSERT INTO security_device_credential_verifiers "
        "(credential_id, device_id, verifier_hash) "
        "SELECT ?, ?, ? WHERE EXISTS ("
        "SELECT 1 FROM security_credentials c "
        "JOIN security_devices d ON d.actor_id = c.actor_id "
        "JOIN security_actors a ON a.actor_id = c.actor_id "
        "WHERE c.credential_id = ? AND d.device_id = ? "
        "AND c.credential_type = 'device-app' "
        "AND c.active = 1 AND c.revoked_at = '' "
        "AND d.active = 1 AND d.revoked_at = '' "
        "AND a.active = 1 AND a.revoked_at = '' "
        "AND a.actor_type = 'service');";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &stmt, nullptr)
        != SQLITE_OK)
        return false;
    const bool bound = bindText(stmt, 1, credentialId) &&
        bindText(stmt, 2, deviceId) &&
        bindText(stmt, 3, verifierHash) &&
        bindText(stmt, 4, credentialId) &&
        bindText(stmt, 5, deviceId);
    const int step = bound ? sqlite3_step(stmt) : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    sqlite3_finalize(stmt);
    return step == SQLITE_DONE && changed == 1;
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
