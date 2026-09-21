#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

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
    std::string lastSeenAt;
    bool active = false;
    bool expired = false;
    bool idleExpired = false;
    bool revoked = false;
};

struct TerminalBrowserSessionCandidate
{
    std::string tokenId;
    std::string sessionId;
    std::string actorId;
    std::string deviceId;
    std::string credentialId;
    std::string terminalAt;
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
        const std::string& tokenId,
        int idleTimeoutSeconds = 0) const;
    std::optional<StoredBrowserSessionCredential> findBySessionId(
        const std::string& sessionId) const;
    std::optional<std::size_t> countEffectiveActiveByActorId(
        const std::string& actorId,
        int idleTimeoutSeconds = 0) const;
    std::optional<bool> touchLastSeenIfDue(
        const std::string& tokenId,
        int minimumIntervalSeconds) const;
    std::optional<std::vector<TerminalBrowserSessionCandidate>>
    findTerminalRetentionCandidates(
        int retentionSeconds,
        int idleTimeoutSeconds,
        std::size_t limit) const;
    std::optional<bool> remainsTerminalRetentionCandidate(
        const TerminalBrowserSessionCandidate& candidate,
        int retentionSeconds,
        int idleTimeoutSeconds) const;
    bool deleteTerminalRetentionCandidate(
        const TerminalBrowserSessionCandidate& candidate);
    bool revokeBySessionId(const std::string& sessionId);
    bool setExpiry(
        const std::string& sessionId,
        const std::string& expiresAt);

    static bool supportsSecretHash(const std::string& secretHash);

private:
    Database& database_;
};