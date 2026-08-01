#pragma once

#include <optional>
#include <string>

class Database;

struct BrowserSessionCredentialRegistration
{
    std::string tokenId;
    std::string sessionId;
    std::string actorId;
    std::string deviceId;
    std::string credentialId;
    std::string issuedFromCredentialId;
    std::string sessionSecretHash;
    std::string csrfSecretHash;
    std::string expiresAt;
};

struct StoredBrowserSessionCredential
{
    std::string tokenId;
    std::string sessionId;
    std::string actorId;
    std::string deviceId;
    std::string credentialId;
    std::string issuedFromCredentialId;
    std::string sessionSecretHash;
    std::string csrfSecretHash;
    std::string expiresAt;
    bool active = false;
    bool expired = false;
    bool revoked = false;
};

class BrowserSessionCredentialRepository
{
public:
    explicit BrowserSessionCredentialRepository(Database& database);

    bool ensureSchema();
    bool insert(const BrowserSessionCredentialRegistration& registration);
    std::optional<StoredBrowserSessionCredential> findByTokenId(
        const std::string& tokenId) const;
    std::optional<StoredBrowserSessionCredential> findResolvedByTokenId(
        const std::string& tokenId) const;
    std::optional<StoredBrowserSessionCredential> findBySessionId(
        const std::string& sessionId) const;
    bool revokeBySessionId(const std::string& sessionId);
    bool setExpiry(
        const std::string& sessionId,
        const std::string& expiresAt);

    static bool supportsSecretHash(const std::string& secretHash);

private:
    Database& database_;
};
