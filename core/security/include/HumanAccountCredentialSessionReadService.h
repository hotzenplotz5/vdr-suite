#pragma once

#include "HumanAccountCredentialSessionReadRepository.h"

#include <string>
#include <vector>

class HumanAccountRepository;
class SecurityIdentityRepository;

enum class HumanAccountCredentialSessionReadStatus
{
    success,
    invalidRequest,
    accountNotFound,
    accountActorInvalid,
    credentialNotFound,
    sessionNotFound,
    storageError,
};

struct HumanAccountCredentialReadResult
{
    HumanAccountCredentialSessionReadStatus status =
        HumanAccountCredentialSessionReadStatus::storageError;
    std::string accountId;
    std::string actorId;
    std::vector<HumanAccountCredentialMetadata> credentials;
};

struct HumanAccountCredentialItemReadResult
{
    HumanAccountCredentialSessionReadStatus status =
        HumanAccountCredentialSessionReadStatus::storageError;
    std::string accountId;
    std::string actorId;
    HumanAccountCredentialMetadata credential;
};

struct HumanAccountSessionItemReadResult
{
    HumanAccountCredentialSessionReadStatus status =
        HumanAccountCredentialSessionReadStatus::storageError;
    std::string accountId;
    std::string actorId;
    HumanAccountSessionMetadata session;
};

struct HumanAccountSessionReadResult
{
    HumanAccountCredentialSessionReadStatus status =
        HumanAccountCredentialSessionReadStatus::storageError;
    std::string accountId;
    std::string actorId;
    std::vector<HumanAccountSessionMetadata> sessions;
};

class HumanAccountCredentialSessionReadService
{
public:
    HumanAccountCredentialSessionReadService(
        HumanAccountRepository& accountRepository,
        SecurityIdentityRepository& identityRepository,
        HumanAccountCredentialSessionReadRepository& metadataRepository);

    HumanAccountCredentialReadResult readCredentials(
        const std::string& accountId) const;

    HumanAccountCredentialItemReadResult readCredential(
        const std::string& accountId,
        const std::string& credentialId) const;

    HumanAccountSessionReadResult readSessions(
        const std::string& accountId) const;

    HumanAccountSessionItemReadResult readSession(
        const std::string& accountId,
        const std::string& sessionId) const;

private:
    HumanAccountCredentialSessionReadStatus resolveActor(
        const std::string& accountId,
        std::string& actorId) const;

    HumanAccountRepository& accountRepository_;
    SecurityIdentityRepository& identityRepository_;
    HumanAccountCredentialSessionReadRepository& metadataRepository_;
};
