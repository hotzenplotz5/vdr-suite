#include "HumanAccountCredentialSessionReadService.h"

#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"

HumanAccountCredentialSessionReadService::
HumanAccountCredentialSessionReadService(
    HumanAccountRepository& accountRepository,
    SecurityIdentityRepository& identityRepository,
    HumanAccountCredentialSessionReadRepository& metadataRepository)
    : accountRepository_(accountRepository),
      identityRepository_(identityRepository),
      metadataRepository_(metadataRepository)
{
}

HumanAccountCredentialSessionReadStatus
HumanAccountCredentialSessionReadService::resolveActor(
    const std::string& accountId,
    std::string& actorId) const
{
    const HumanAccountLookupResult account =
        accountRepository_.findByAccountId(accountId);
    switch (account.status)
    {
        case HumanAccountRepositoryStatus::invalid:
            return HumanAccountCredentialSessionReadStatus::invalidRequest;
        case HumanAccountRepositoryStatus::notFound:
            return HumanAccountCredentialSessionReadStatus::accountNotFound;
        case HumanAccountRepositoryStatus::revisionConflict:
        case HumanAccountRepositoryStatus::storageError:
            return HumanAccountCredentialSessionReadStatus::storageError;
        case HumanAccountRepositoryStatus::ok:
            break;
    }

    const auto actor =
        identityRepository_.findActor(account.account.actorId);
    if (!actor.has_value() || actor->type != ActorType::User)
        return HumanAccountCredentialSessionReadStatus::accountActorInvalid;

    actorId = account.account.actorId;
    return HumanAccountCredentialSessionReadStatus::success;
}

HumanAccountCredentialReadResult
HumanAccountCredentialSessionReadService::readCredentials(
    const std::string& accountId) const
{
    HumanAccountCredentialReadResult result;
    result.accountId = accountId;
    const auto resolved = resolveActor(accountId, result.actorId);
    if (resolved != HumanAccountCredentialSessionReadStatus::success)
    {
        result.status = resolved;
        return result;
    }

    const auto credentials =
        metadataRepository_.listCredentialsByActorId(result.actorId);
    if (!credentials.has_value())
        return result;

    result.credentials = *credentials;
    result.status = HumanAccountCredentialSessionReadStatus::success;
    return result;
}

HumanAccountSessionReadResult
HumanAccountCredentialSessionReadService::readSessions(
    const std::string& accountId) const
{
    HumanAccountSessionReadResult result;
    result.accountId = accountId;
    const auto resolved = resolveActor(accountId, result.actorId);
    if (resolved != HumanAccountCredentialSessionReadStatus::success)
    {
        result.status = resolved;
        return result;
    }

    const auto sessions =
        metadataRepository_.listSessionsByActorId(result.actorId);
    if (!sessions.has_value())
        return result;

    result.sessions = *sessions;
    result.status = HumanAccountCredentialSessionReadStatus::success;
    return result;
}
