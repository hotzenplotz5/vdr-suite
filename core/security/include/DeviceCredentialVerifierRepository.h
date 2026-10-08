#pragma once

#include <optional>
#include <string>

class Database;

// Verifier binding only. Canonical lifecycle remains security_credentials
// and security_devices in SecurityIdentityRepository.
struct StoredDeviceCredentialVerifier
{
    std::string credentialId;
    std::string deviceId;
    std::string verifierHash;
};

class DeviceCredentialVerifierRepository
{
public:
    explicit DeviceCredentialVerifierRepository(Database& database);
    bool ensureSchema();
    // Only legal inside the issuer's transaction. Never persist plaintext.
    bool insertInActiveTransaction(
        const std::string& credentialId,
        const std::string& deviceId,
        const std::string& verifierHash);
    std::optional<StoredDeviceCredentialVerifier> findByCredentialId(
        const std::string& credentialId) const;

private:
    Database& database_;
};
