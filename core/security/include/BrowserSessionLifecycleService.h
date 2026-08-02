#pragma once

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

private:
    Database& database_;
    SecurityIdentityRepository& identityRepository_;
    BrowserSessionCredentialRepository& credentialRepository_;
};
