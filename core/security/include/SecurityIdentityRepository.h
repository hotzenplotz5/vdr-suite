#pragma once

#include "SecurityIdentity.h"

#include <optional>
#include <string>

class Database;

struct StoredActorIdentity
{
    std::string actorId;
    ActorType type = ActorType::Anonymous;
    std::string displayName;
    bool active = false;
    bool revoked = false;
};

struct StoredDeviceIdentity
{
    std::string deviceId;
    std::string actorId;
    std::string displayName;
    bool active = false;
    bool revoked = false;
};

struct StoredSessionIdentity
{
    std::string sessionId;
    std::string actorId;
    std::string deviceId;
    bool active = false;
    bool expired = false;
    bool revoked = false;
};

struct StoredCredentialIdentity
{
    std::string credentialId;
    std::string actorId;
    std::string credentialType;
    bool active = false;
    bool expired = false;
    bool revoked = false;
};

class SecurityIdentityRepository
{
public:
    explicit SecurityIdentityRepository(Database& database);

    bool ensureSchema();
    bool ensureCompatibilityIdentity(
        const std::string& actorId,
        ActorType actorType,
        const std::string& actorDisplayName,
        const std::string& deviceId,
        const std::string& sessionId,
        const std::string& credentialId);

    bool createSessionCredential(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& deviceId,
        const std::string& credentialId,
        const std::string& credentialType,
        const std::string& expiresAt,
        const std::string& issuedFromCredentialId);

    std::optional<StoredActorIdentity> findActor(
        const std::string& actorId) const;
    std::optional<StoredDeviceIdentity> findDevice(
        const std::string& deviceId) const;
    std::optional<StoredSessionIdentity> findSession(
        const std::string& sessionId) const;
    std::optional<StoredCredentialIdentity> findCredential(
        const std::string& credentialId) const;

    bool revokeActor(const std::string& actorId);
    bool revokeDevice(const std::string& deviceId);
    bool revokeSession(const std::string& sessionId);
    bool revokeCredential(const std::string& credentialId);
    bool setSessionExpiry(
        const std::string& sessionId,
        const std::string& expiresAt);
    bool setCredentialExpiry(
        const std::string& credentialId,
        const std::string& expiresAt);
    bool deleteSessionIfUnreferenced(
        const std::string& sessionId,
        const std::string& actorId,
        const std::string& deviceId);
    bool deleteBrowserSessionCredentialIfUnreferenced(
        const std::string& credentialId,
        const std::string& actorId);

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
    bool markRevoked(
        const std::string& table,
        const std::string& keyColumn,
        const std::string& keyValue);
    bool updateText(
        const std::string& table,
        const std::string& keyColumn,
        const std::string& keyValue,
        const std::string& valueColumn,
        const std::string& value);

    Database& database_;
};