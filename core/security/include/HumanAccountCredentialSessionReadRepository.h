#pragma once

#include <optional>
#include <string>
#include <vector>

class Database;

struct HumanAccountCredentialMetadata
{
    std::string credentialId;
    std::string credentialType;
    bool active = false;
    bool expired = false;
    bool revoked = false;
    std::string expiresAt;
    std::string createdAt;
};

struct HumanAccountSessionMetadata
{
    std::string sessionId;
    std::string deviceId;
    std::string issuedFromCredentialId;
    bool active = false;
    bool expired = false;
    bool revoked = false;
    std::string expiresAt;
    std::string lastSeenAt;
    std::string createdAt;
    std::string browserCredentialId;
    std::string resourceRevision;
};

class HumanAccountCredentialSessionReadRepository
{
public:
    explicit HumanAccountCredentialSessionReadRepository(
        Database& database);

    std::optional<std::vector<HumanAccountCredentialMetadata>>
    listCredentialsByActorId(const std::string& actorId) const;

    std::optional<std::vector<HumanAccountSessionMetadata>>
    listSessionsByActorId(const std::string& actorId) const;

private:
    Database& database_;
};
