#pragma once

#include <optional>
#include <string>

class Database;

struct StoredBasicCredentialVerifier
{
    std::string credentialId;
    std::string loginName;
    std::string passwordHash;
};

class CredentialVerifierRepository
{
public:
    explicit CredentialVerifierRepository(Database& database);

    bool ensureSchema();
    bool ensureVerifier(
        const std::string& credentialId,
        const std::string& loginName,
        const std::string& passwordHash);
    std::optional<StoredBasicCredentialVerifier> findByLogin(
        const std::string& loginName) const;

private:
    Database& database_;
};
