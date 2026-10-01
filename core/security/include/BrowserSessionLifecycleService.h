#pragma once

#include <cstddef>
#include <optional>
#include <string>

class BrowserSessionCredentialRepository;
class Database;
class SecurityIdentityRepository;

class BrowserSessionLifecycleService
{
public:
    BrowserSessionLifecycleService(
        Database& database,
        SecurityIdentityRepository& identityRepository,
        BrowserSessionCredentialRepository& credentialRepository);

    bool revoke(
        const std::string& sessionId,
        const std::string& credentialId);

    std::optional<std::size_t> revokeAllForActorInActiveTransaction(
        const std::string& actorId);

private:
    bool revokeInActiveTransaction(
        const std::string& sessionId,
        const std::string& credentialId);

    Database& database_;
    SecurityIdentityRepository& identityRepository_;
    BrowserSessionCredentialRepository& credentialRepository_;
};
