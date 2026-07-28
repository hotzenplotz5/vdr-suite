#pragma once

#include "SecurityIdentity.h"

#include <string>

class Database;
class SecurityIdentityRepository;

class SecurityIdentityProvisioningRepository
{
public:
    explicit SecurityIdentityProvisioningRepository(Database& database);
    ~SecurityIdentityProvisioningRepository();

    SecurityIdentityProvisioningRepository(
        const SecurityIdentityProvisioningRepository&) = delete;
    SecurityIdentityProvisioningRepository& operator=(
        const SecurityIdentityProvisioningRepository&) = delete;

    bool ensureIdentity(
        const std::string& actorId,
        ActorType actorType,
        const std::string& actorDisplayName,
        const std::string& deviceId,
        const std::string& deviceDisplayName,
        const std::string& sessionId,
        const std::string& credentialId,
        const std::string& credentialType);

private:
    bool insertActorIfMissing(
        const std::string& actorId,
        const std::string& actorType,
        const std::string& displayName);
    bool insertDeviceIfMissing(
        const std::string& deviceId,
        const std::string& actorId,
        const std::string& displayName);
    bool insertSessionIfMissing(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& deviceId);
    bool insertCredentialIfMissing(
        const std::string& credentialId,
        const std::string& actorId,
        const std::string& credentialType);
    bool executeThreeText(
        const char* sql,
        const std::string& first,
        const std::string& second,
        const std::string& third);

    Database& database_;
    SecurityIdentityRepository* identityRepository_;
};
