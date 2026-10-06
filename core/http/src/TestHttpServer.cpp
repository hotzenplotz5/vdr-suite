#include "TestHttpServer.h"

#include "ApiRouter.h"
#include "BrowserSessionCsrfRecoveryService.h"
#include "ContinueWatchingSecurityRequest.h"
#include "FirstAdminClaimHttpService.h"
#include "SecurityConfiguration.h"
#include "SeriesArtworkSettingsSecurityRequest.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

#include "TestHttpServerPaths.inc"
#include "TestHttpServerAssets.inc"
#include "TestHttpServerRoutes.inc"

constexpr const char* BrowserSessionCurrentPath =
    "/api/security/browser-sessions/current";

std::string pathWithoutQuery(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos
        ? target
        : target.substr(0, query);
}

bool sameHeaderName(
    const std::string& left,
    const std::string& right)
{
    if (left.size() != right.size())
    {
        return false;
    }

    for (std::size_t index = 0;
         index < left.size();
         ++index)
    {
        const unsigned char lhs =
            static_cast<unsigned char>(left[index]);
        const unsigned char rhs =
            static_cast<unsigned char>(right[index]);

        if (std::tolower(lhs) != std::tolower(rhs))
        {
            return false;
        }
    }

    return true;
}

std::string requestHeaderValue(
    const HttpServerRequest& request,
    const std::string& name)
{
    for (const auto& header : request.headers)
    {
        if (sameHeaderName(header.first, name))
        {
            return header.second;
        }
    }

    return "";
}

std::string hbbtvClientContext(
    const RequestSecurityContext& context)
{
    if (context.device.has_value() &&
        !context.device->deviceId.empty())
    {
        return context.device->deviceId;
    }
    if (context.session.has_value() &&
        !context.session->sessionId.empty())
    {
        return context.session->sessionId;
    }
    if (context.credential.has_value() &&
        !context.credential->credentialId.empty())
    {
        return context.credential->credentialId;
    }
    return context.actor.actorId;
}

std::string securityDatabasePath()
{
    const char* configured =
        std::getenv("VDR_SUITE_SECURITY_DATABASE_PATH");

    if (configured != nullptr &&
        configured[0] != '\0')
    {
        return configured;
    }

    configured =
        std::getenv("VDR_SUITE_DATABASE_PATH");

    if (configured != nullptr &&
        configured[0] != '\0')
    {
        return configured;
    }

    return "/tmp/vdr-suite-test.db";
}

}

TestHttpServer::TestHttpServer(ApiRouter& apiRouter)
    : apiRouter_(apiRouter),
      securityDatabase_(std::make_unique<Database>())
{
    if (!securityDatabase_->open(
            securityDatabasePath()))
    {
        return;
    }

    accountabilityEventRepository_ =
        std::make_unique<AccountabilityEventRepository>(
            *securityDatabase_);

    if (!accountabilityEventRepository_->ensureSchema())
    {
        return;
    }

    devicePairingRequestRepository_ =
        std::make_unique<DevicePairingRequestRepository>(
            *securityDatabase_);
    if (!devicePairingRequestRepository_->ensureSchema())
    {
        return;
    }
    devicePairingRequestService_ =
        std::make_unique<DevicePairingRequestService>(
            *securityDatabase_,
            *devicePairingRequestRepository_,
            *accountabilityEventRepository_);

    securityIdentityRepository_ =
        std::make_unique<SecurityIdentityRepository>(
            *securityDatabase_);

    if (!securityIdentityRepository_->ensureSchema())
    {
        return;
    }

    securityIdentityProvisioningRepository_ =
        std::make_unique<SecurityIdentityProvisioningRepository>(
            *securityDatabase_);

    credentialVerifierRepository_ =
        std::make_unique<CredentialVerifierRepository>(
            *securityDatabase_);

    if (!credentialVerifierRepository_->ensureSchema())
    {
        return;
    }

    browserSessionCredentialRepository_ =
        std::make_unique<BrowserSessionCredentialRepository>(
            *securityDatabase_);

    if (!browserSessionCredentialRepository_->ensureSchema())
    {
        return;
    }

    securityPermissionGrantRepository_ =
        std::make_unique<SecurityPermissionGrantRepository>(
            *securityDatabase_);

    if (!securityPermissionGrantRepository_->ensureSchema())
    {
        return;
    }

    humanAccountRepository_ =
        std::make_unique<HumanAccountRepository>(
            *securityDatabase_);
    if (!humanAccountRepository_->ensureSchema())
    {
        return;
    }

    humanAccountCreationRepository_ =
        std::make_unique<HumanAccountCreationRepository>(
            *securityDatabase_);
    if (!humanAccountCreationRepository_->ensureSchema())
    {
        return;
    }

    firstAdminBootstrapRepository_ =
        std::make_unique<FirstAdminBootstrapRepository>(
            *securityDatabase_);
    if (!firstAdminBootstrapRepository_->ensureSchema())
    {
        return;
    }

    firstAdminClaimService_ =
        std::make_unique<FirstAdminClaimService>(
            *securityDatabase_,
            *firstAdminBootstrapRepository_,
            *securityIdentityProvisioningRepository_,
            *humanAccountRepository_,
            *credentialVerifierRepository_,
            *securityPermissionGrantRepository_,
            *accountabilityEventRepository_);
    firstAdminClaimHttpService_ =
        std::make_unique<FirstAdminClaimHttpService>(
            *firstAdminClaimService_);

    const SecurityConfiguration configuration =
        SecurityConfiguration::fromEnvironment();
    if (!configuration.browserSessionLifetime.valid() ||
        !configuration.browserSessionConcurrency.valid() ||
        !configuration.browserSessionIdle.valid() ||
        !configuration.browserSessionRetention.valid() ||
        (configuration.managedBasic.hasAnyConfiguration() &&
         (!configuration.managedBasic.complete() ||
          !ManagedBasicAuthenticator::supportsPasswordHash(
              configuration.managedBasic.passwordHash))))
    {
        return;
    }

    browserSessionRetentionService_ =
        std::make_unique<BrowserSessionRetentionService>(
            *securityDatabase_,
            *browserSessionCredentialRepository_,
            *securityIdentityRepository_,
            *accountabilityEventRepository_);
    if (!browserSessionRetentionService_->cleanup(
            configuration.browserSessionRetention,
            configuration.browserSessionIdle))
    {
        return;
    }

    if (configuration.managedBasic.hasAnyConfiguration())
    {
        if (!securityIdentityProvisioningRepository_->ensureIdentity(
                configuration.managedBasic.actorId,
                ActorType::User,
                configuration.managedBasic.actorDisplayName,
                configuration.managedBasic.deviceId,
                "Managed Basic client",
                configuration.managedBasic.sessionId,
                configuration.managedBasic.credentialId,
                "managed-basic"))
        {
            return;
        }

        if (!credentialVerifierRepository_->ensureVerifier(
                configuration.managedBasic.credentialId,
                configuration.managedBasic.username,
                configuration.managedBasic.passwordHash))
        {
            return;
        }

        managedBasicAuthenticator_ =
            std::make_unique<ManagedBasicAuthenticator>(
                configuration.managedBasic,
                *credentialVerifierRepository_);
    }

    humanPasswordBrowserAuthenticator_ =
        std::make_unique<HumanPasswordBrowserAuthenticator>(
            *credentialVerifierRepository_,
            *securityIdentityRepository_,
            *humanAccountRepository_,
            *securityIdentityProvisioningRepository_);

    persistentIdentityResolver_ =
        std::make_unique<PersistentIdentityResolver>(
            *securityIdentityRepository_);

    const int browserIdleTimeout = configuration.browserSessionIdle.valid()
        ? configuration.browserSessionIdle.timeoutSeconds
        : -1;

    browserSessionAuthenticator_ =
        std::make_unique<BrowserSessionAuthenticator>(
            *browserSessionCredentialRepository_,
            *securityPermissionGrantRepository_,
            browserIdleTimeout,
            BrowserSessionIdleConfiguration::LastSeenWriteIntervalSeconds,
            humanAccountRepository_.get());

    browserSessionIssuanceService_ =
        std::make_unique<BrowserSessionIssuanceService>(
            *securityDatabase_,
            *securityIdentityRepository_,
            *browserSessionCredentialRepository_);

    browserSessionLifecycleService_ =
        std::make_unique<BrowserSessionLifecycleService>(
            *securityDatabase_,
            *securityIdentityRepository_,
            *browserSessionCredentialRepository_);

    humanAccountAdministrationRepository_ =
        std::make_unique<HumanAccountAdministrationRepository>(
            *securityDatabase_);
    humanAccountAdministrationService_ =
        std::make_unique<HumanAccountAdministrationService>(
            *securityDatabase_,
            *humanAccountRepository_,
            *humanAccountAdministrationRepository_,
            *securityIdentityRepository_,
            *browserSessionLifecycleService_,
            *accountabilityEventRepository_);

    humanAccountCreationService_ =
        std::make_unique<HumanAccountCreationService>(
            *securityDatabase_,
            *securityIdentityProvisioningRepository_,
            *humanAccountRepository_,
            *humanAccountCreationRepository_,
            *credentialVerifierRepository_,
            *accountabilityEventRepository_);

    humanAccountGrantAdministrationService_ =
        std::make_unique<HumanAccountGrantAdministrationService>(
            *securityDatabase_,
            *humanAccountRepository_,
            *securityIdentityRepository_,
            *securityPermissionGrantRepository_,
            *humanAccountAdministrationRepository_,
            *accountabilityEventRepository_);

    humanAccountCredentialSessionReadRepository_ =
        std::make_unique<HumanAccountCredentialSessionReadRepository>(
            *securityDatabase_);
    humanAccountCredentialSessionReadService_ =
        std::make_unique<HumanAccountCredentialSessionReadService>(
            *humanAccountRepository_,
            *securityIdentityRepository_,
            *humanAccountCredentialSessionReadRepository_);
    humanAccountCredentialAdministrationService_ =
        std::make_unique<HumanAccountCredentialAdministrationService>(
            *securityDatabase_,
            *humanAccountCredentialSessionReadService_,
            *humanAccountAdministrationRepository_,
            *securityIdentityRepository_,
            *browserSessionCredentialRepository_,
            *browserSessionLifecycleService_,
            *accountabilityEventRepository_);
    humanAccountSessionAdministrationService_ =
        std::make_unique<HumanAccountSessionAdministrationService>(
            *securityDatabase_,
            *humanAccountCredentialSessionReadService_,
            *browserSessionLifecycleService_,
            *accountabilityEventRepository_);

    PublicApiRuntime::instance().registerDevicePairingCreate(
        [this](const PublicDevicePairingCreateRequest& request)
        {
            PublicDevicePairingCreateResult result;
            if (!devicePairingRequestService_)
                return result;

            DevicePairingIssueRequest serviceRequest;
            serviceRequest.client.displayName =
                request.client.displayName;
            serviceRequest.client.clientKind =
                request.client.clientKind;
            serviceRequest.client.appVersion =
                request.client.appVersion;
            serviceRequest.requestId =
                request.requestId;
            serviceRequest.correlationId =
                request.correlationId;

            DevicePairingIssueResult created =
                devicePairingRequestService_->issue(
                    serviceRequest);
            switch (created.status)
            {
                case DevicePairingIssueStatus::issued:
                    result.status =
                        PublicDevicePairingCreateStatus::created;
                    break;
                case DevicePairingIssueStatus::invalidRequest:
                    result.status =
                        PublicDevicePairingCreateStatus::invalid;
                    return result;
                case DevicePairingIssueStatus::entropyUnavailable:
                    result.status =
                        PublicDevicePairingCreateStatus::
                            entropyUnavailable;
                    return result;
                case DevicePairingIssueStatus::hashingUnavailable:
                    result.status =
                        PublicDevicePairingCreateStatus::
                            hashingUnavailable;
                    return result;
                case DevicePairingIssueStatus::storageError:
                    result.status =
                        PublicDevicePairingCreateStatus::unavailable;
                    return result;
            }

            if (!created.pairing.has_value())
            {
                result.status =
                    PublicDevicePairingCreateStatus::unavailable;
                return result;
            }

            result.resource.pairingRequestId =
                created.pairing->resource.pairingRequestId;
            result.resource.client.displayName =
                created.pairing->resource.client.displayName;
            result.resource.client.clientKind =
                created.pairing->resource.client.clientKind;
            result.resource.client.appVersion =
                created.pairing->resource.client.appVersion;
            result.resource.state =
                created.pairing->resource.state;
            result.resource.expiresAt =
                created.pairing->resource.expiresAt;
            result.resource.pollIntervalSeconds =
                created.pairing->resource.pollIntervalSeconds;
            result.userCode =
                std::move(created.pairing->userCode);
            result.pairingToken =
                std::move(created.pairing->pairingToken);
            created.pairing->clearBootstrapMaterial();
            return result;
        });

    PublicApiRuntime::instance().registerDevicePairingLookup(
        [this](const PublicDevicePairingLookupRequest& request)
        {
            PublicDevicePairingLookupResult result;
            if (!devicePairingRequestService_)
                return result;

            DevicePairingPollRequest serviceRequest;
            serviceRequest.pairingRequestId =
                request.pairingRequestId;
            serviceRequest.pairingToken =
                request.pairingToken;
            const DevicePairingPollResult found =
                devicePairingRequestService_->poll(
                    serviceRequest);

            switch (found.status)
            {
                case DevicePairingPollStatus::ok:
                    result.status =
                        PublicDevicePairingLookupStatus::ok;
                    break;
                case DevicePairingPollStatus::invalidRequest:
                    result.status =
                        PublicDevicePairingLookupStatus::invalid;
                    return result;
                case DevicePairingPollStatus::notFound:
                    result.status =
                        PublicDevicePairingLookupStatus::notFound;
                    return result;
                case DevicePairingPollStatus::unauthorized:
                    result.status =
                        PublicDevicePairingLookupStatus::unauthorized;
                    return result;
                case DevicePairingPollStatus::expired:
                    result.status =
                        PublicDevicePairingLookupStatus::expired;
                    return result;
                case DevicePairingPollStatus::unavailable:
                    result.status =
                        PublicDevicePairingLookupStatus::unavailable;
                    return result;
            }

            result.resource.pairingRequestId =
                found.resource.pairingRequestId;
            result.resource.client.displayName =
                found.resource.client.displayName;
            result.resource.client.clientKind =
                found.resource.client.clientKind;
            result.resource.client.appVersion =
                found.resource.client.appVersion;
            result.resource.state =
                found.resource.state;
            result.resource.expiresAt =
                found.resource.expiresAt;
            result.resource.pollIntervalSeconds =
                found.resource.pollIntervalSeconds;
            return result;
        });

    PublicApiRuntime::instance().registerAccountCredentialLookup(
        [this](const std::string& accountId)
        {
            PublicAccountCredentialCollectionResult result;
            if (!humanAccountCredentialSessionReadService_)
                return result;
            const HumanAccountCredentialReadResult found =
                humanAccountCredentialSessionReadService_->
                    readCredentials(accountId);
            switch (found.status)
            {
                case HumanAccountCredentialSessionReadStatus::success:
                    result.status =
                        PublicAccountSecurityMetadataStatus::ok;
                    break;
                case HumanAccountCredentialSessionReadStatus::invalidRequest:
                    result.status =
                        PublicAccountSecurityMetadataStatus::invalid;
                    return result;
                case HumanAccountCredentialSessionReadStatus::accountNotFound:
                    result.status =
                        PublicAccountSecurityMetadataStatus::notFound;
                    return result;
                case HumanAccountCredentialSessionReadStatus::credentialNotFound:
                case HumanAccountCredentialSessionReadStatus::sessionNotFound:
                case HumanAccountCredentialSessionReadStatus::accountActorInvalid:
                case HumanAccountCredentialSessionReadStatus::storageError:
                    result.status =
                        PublicAccountSecurityMetadataStatus::unavailable;
                    return result;
            }
            result.collection.accountId = found.accountId;
            result.collection.actorId = found.actorId;
            for (const HumanAccountCredentialMetadata& credential :
                 found.credentials)
            {
                PublicAccountCredentialItem item;
                item.credentialId = credential.credentialId;
                item.credentialType = credential.credentialType;
                item.active = credential.active;
                item.expired = credential.expired;
                item.revoked = credential.revoked;
                item.expiresAt = credential.expiresAt;
                item.createdAt = credential.createdAt;
                result.collection.credentials.push_back(
                    std::move(item));
            }
            return result;
        });

    PublicApiRuntime::instance().registerAccountCredentialItemLookup(
        [this](
            const std::string& accountId,
            const std::string& credentialId)
        {
            PublicAccountCredentialLookupResult result;
            if (!humanAccountCredentialSessionReadService_)
                return result;

            const HumanAccountCredentialItemReadResult found =
                humanAccountCredentialSessionReadService_->readCredential(
                    accountId,
                    credentialId);

            switch (found.status)
            {
                case HumanAccountCredentialSessionReadStatus::success:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::ok;
                    break;
                case HumanAccountCredentialSessionReadStatus::invalidRequest:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::invalid;
                    return result;
                case HumanAccountCredentialSessionReadStatus::accountNotFound:
                case HumanAccountCredentialSessionReadStatus::credentialNotFound:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::notFound;
                    return result;
                case HumanAccountCredentialSessionReadStatus::sessionNotFound:
                case HumanAccountCredentialSessionReadStatus::accountActorInvalid:
                case HumanAccountCredentialSessionReadStatus::storageError:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::unavailable;
                    return result;
            }

            result.resource.accountId = found.accountId;
            result.resource.actorId = found.actorId;
            result.resource.resourceRevision =
                found.credential.resourceRevision;
            result.resource.credential.credentialId =
                found.credential.credentialId;
            result.resource.credential.credentialType =
                found.credential.credentialType;
            result.resource.credential.active =
                found.credential.active;
            result.resource.credential.expired =
                found.credential.expired;
            result.resource.credential.revoked =
                found.credential.revoked;
            result.resource.credential.expiresAt =
                found.credential.expiresAt;
            result.resource.credential.createdAt =
                found.credential.createdAt;
            return result;
        });

    PublicApiRuntime::instance().registerAccountCredentialMutation(
        [this](const PublicAccountCredentialMutationRequest& request)
        {
            PublicAccountCredentialMutationResult result;
            if (!humanAccountCredentialAdministrationService_ ||
                !securityIdentityRepository_)
            {
                return result;
            }

            const std::optional<StoredActorIdentity> actor =
                securityIdentityRepository_->findActor(
                    request.actorRef);
            if (!actor.has_value() ||
                actor->type == ActorType::Anonymous ||
                !actor->active ||
                actor->revoked)
            {
                return result;
            }

            HumanAccountCredentialAdministrationContext context;
            context.actorId = request.actorRef;
            context.requestId = request.requestId;
            context.correlationId = request.correlationId;

            const HumanAccountCredentialAdministrationResult mutated =
                humanAccountCredentialAdministrationService_->revoke(
                    context,
                    request.accountId,
                    request.credentialId,
                    request.expectedResourceRevision);

            switch (mutated.status)
            {
                case HumanAccountCredentialAdministrationStatus::success:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::ok;
                    break;
                case HumanAccountCredentialAdministrationStatus::invalidRequest:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::invalid;
                    return result;
                case HumanAccountCredentialAdministrationStatus::accountNotFound:
                case HumanAccountCredentialAdministrationStatus::credentialNotFound:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::notFound;
                    return result;
                case HumanAccountCredentialAdministrationStatus::credentialInvalid:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::validationError;
                    return result;
                case HumanAccountCredentialAdministrationStatus::revisionConflict:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::revisionConflict;
                    break;
                case HumanAccountCredentialAdministrationStatus::finalAdministrator:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::finalAdministrator;
                    break;
                case HumanAccountCredentialAdministrationStatus::accountActorInvalid:
                case HumanAccountCredentialAdministrationStatus::entropyUnavailable:
                case HumanAccountCredentialAdministrationStatus::storageError:
                    result.status =
                        PublicAccountCredentialAdministrationStatus::unavailable;
                    return result;
            }

            result.resource.accountId = mutated.accountId;
            result.resource.actorId = mutated.actorId;
            result.resource.resourceRevision =
                mutated.credential.resourceRevision;
            result.resource.credential.credentialId =
                mutated.credential.credentialId;
            result.resource.credential.credentialType =
                mutated.credential.credentialType;
            result.resource.credential.active =
                mutated.credential.active;
            result.resource.credential.expired =
                mutated.credential.expired;
            result.resource.credential.revoked =
                mutated.credential.revoked;
            result.resource.credential.expiresAt =
                mutated.credential.expiresAt;
            result.resource.credential.createdAt =
                mutated.credential.createdAt;
            return result;
        });

    PublicApiRuntime::instance().registerAccountSessionLookup(
        [this](const std::string& accountId)
        {
            PublicAccountSessionCollectionResult result;
            if (!humanAccountCredentialSessionReadService_)
                return result;
            const HumanAccountSessionReadResult found =
                humanAccountCredentialSessionReadService_->
                    readSessions(accountId);
            switch (found.status)
            {
                case HumanAccountCredentialSessionReadStatus::success:
                    result.status =
                        PublicAccountSecurityMetadataStatus::ok;
                    break;
                case HumanAccountCredentialSessionReadStatus::invalidRequest:
                    result.status =
                        PublicAccountSecurityMetadataStatus::invalid;
                    return result;
                case HumanAccountCredentialSessionReadStatus::accountNotFound:
                    result.status =
                        PublicAccountSecurityMetadataStatus::notFound;
                    return result;
                case HumanAccountCredentialSessionReadStatus::credentialNotFound:
                case HumanAccountCredentialSessionReadStatus::sessionNotFound:
                case HumanAccountCredentialSessionReadStatus::accountActorInvalid:
                case HumanAccountCredentialSessionReadStatus::storageError:
                    result.status =
                        PublicAccountSecurityMetadataStatus::unavailable;
                    return result;
            }
            result.collection.accountId = found.accountId;
            result.collection.actorId = found.actorId;
            for (const HumanAccountSessionMetadata& session :
                 found.sessions)
            {
                PublicAccountSessionItem item;
                item.sessionId = session.sessionId;
                item.deviceId = session.deviceId;
                item.issuedFromCredentialId =
                    session.issuedFromCredentialId;
                item.active = session.active;
                item.expired = session.expired;
                item.revoked = session.revoked;
                item.expiresAt = session.expiresAt;
                item.lastSeenAt = session.lastSeenAt;
                item.createdAt = session.createdAt;
                result.collection.sessions.push_back(
                    std::move(item));
            }
            return result;
        });

    PublicApiRuntime::instance().registerAccountSessionItemLookup(
        [this](
            const std::string& accountId,
            const std::string& sessionId)
        {
            PublicAccountSessionLookupResult result;
            if (!humanAccountCredentialSessionReadService_)
                return result;

            const HumanAccountSessionItemReadResult found =
                humanAccountCredentialSessionReadService_->readSession(
                    accountId,
                    sessionId);

            switch (found.status)
            {
                case HumanAccountCredentialSessionReadStatus::success:
                    result.status =
                        PublicAccountSessionAdministrationStatus::ok;
                    break;
                case HumanAccountCredentialSessionReadStatus::invalidRequest:
                    result.status =
                        PublicAccountSessionAdministrationStatus::invalid;
                    return result;
                case HumanAccountCredentialSessionReadStatus::accountNotFound:
                case HumanAccountCredentialSessionReadStatus::sessionNotFound:
                    result.status =
                        PublicAccountSessionAdministrationStatus::notFound;
                    return result;
                case HumanAccountCredentialSessionReadStatus::credentialNotFound:
                case HumanAccountCredentialSessionReadStatus::accountActorInvalid:
                case HumanAccountCredentialSessionReadStatus::storageError:
                    result.status =
                        PublicAccountSessionAdministrationStatus::unavailable;
                    return result;
            }

            result.resource.accountId = found.accountId;
            result.resource.actorId = found.actorId;
            result.resource.resourceRevision =
                found.session.resourceRevision;
            result.resource.session.sessionId =
                found.session.sessionId;
            result.resource.session.deviceId =
                found.session.deviceId;
            result.resource.session.issuedFromCredentialId =
                found.session.issuedFromCredentialId;
            result.resource.session.active = found.session.active;
            result.resource.session.expired = found.session.expired;
            result.resource.session.revoked = found.session.revoked;
            result.resource.session.expiresAt = found.session.expiresAt;
            result.resource.session.lastSeenAt = found.session.lastSeenAt;
            result.resource.session.createdAt = found.session.createdAt;
            return result;
        });

    PublicApiRuntime::instance().registerAccountSessionMutation(
        [this](const PublicAccountSessionMutationRequest& request)
        {
            PublicAccountSessionMutationResult result;
            if (!humanAccountSessionAdministrationService_ ||
                !securityIdentityRepository_)
            {
                return result;
            }

            const std::optional<StoredActorIdentity> actor =
                securityIdentityRepository_->findActor(
                    request.actorRef);
            if (!actor.has_value() ||
                actor->type == ActorType::Anonymous ||
                !actor->active ||
                actor->revoked)
            {
                return result;
            }

            HumanAccountSessionAdministrationContext context;
            context.actorId = request.actorRef;
            context.requestId = request.requestId;
            context.correlationId = request.correlationId;

            const HumanAccountSessionAdministrationResult mutated =
                humanAccountSessionAdministrationService_->revoke(
                    context,
                    request.accountId,
                    request.sessionId,
                    request.expectedResourceRevision);

            switch (mutated.status)
            {
                case HumanAccountSessionAdministrationStatus::success:
                    result.status =
                        PublicAccountSessionAdministrationStatus::ok;
                    break;
                case HumanAccountSessionAdministrationStatus::invalidRequest:
                    result.status =
                        PublicAccountSessionAdministrationStatus::invalid;
                    return result;
                case HumanAccountSessionAdministrationStatus::accountNotFound:
                case HumanAccountSessionAdministrationStatus::sessionNotFound:
                    result.status =
                        PublicAccountSessionAdministrationStatus::notFound;
                    return result;
                case HumanAccountSessionAdministrationStatus::revisionConflict:
                    result.status =
                        PublicAccountSessionAdministrationStatus::revisionConflict;
                    break;
                case HumanAccountSessionAdministrationStatus::accountActorInvalid:
                case HumanAccountSessionAdministrationStatus::entropyUnavailable:
                case HumanAccountSessionAdministrationStatus::storageError:
                    result.status =
                        PublicAccountSessionAdministrationStatus::unavailable;
                    return result;
            }

            result.resource.accountId = mutated.accountId;
            result.resource.actorId = mutated.actorId;
            result.resource.resourceRevision =
                mutated.session.resourceRevision;
            result.resource.session.sessionId =
                mutated.session.sessionId;
            result.resource.session.deviceId =
                mutated.session.deviceId;
            result.resource.session.issuedFromCredentialId =
                mutated.session.issuedFromCredentialId;
            result.resource.session.active = mutated.session.active;
            result.resource.session.expired = mutated.session.expired;
            result.resource.session.revoked = mutated.session.revoked;
            result.resource.session.expiresAt = mutated.session.expiresAt;
            result.resource.session.lastSeenAt = mutated.session.lastSeenAt;
            result.resource.session.createdAt = mutated.session.createdAt;
            return result;
        });

    PublicApiRuntime::instance().registerAccountGrantLookup(
        [this](const std::string& accountId)
        {
            PublicAccountGrantLookupResult result;
            if (!humanAccountGrantAdministrationService_)
            {
                return result;
            }

            const HumanAccountGrantAdministrationResult found =
                humanAccountGrantAdministrationService_->read(
                    accountId);

            switch (found.status)
            {
                case HumanAccountGrantAdministrationStatus::success:
                    result.status =
                        PublicAccountGrantStatus::ok;
                    break;
                case HumanAccountGrantAdministrationStatus::accountNotFound:
                    result.status =
                        PublicAccountGrantStatus::notFound;
                    return result;
                case HumanAccountGrantAdministrationStatus::invalidGrant:
                    result.status =
                        PublicAccountGrantStatus::invalid;
                    return result;
                case HumanAccountGrantAdministrationStatus::accountActorInvalid:
                case HumanAccountGrantAdministrationStatus::revisionConflict:
                case HumanAccountGrantAdministrationStatus::finalAdministrator:
                case HumanAccountGrantAdministrationStatus::entropyUnavailable:
                case HumanAccountGrantAdministrationStatus::storageError:
                    result.status =
                        PublicAccountGrantStatus::unavailable;
                    return result;
            }

            result.grantSet.accountId =
                found.grantSet.accountId;
            result.grantSet.actorId =
                found.grantSet.actorId;
            result.grantSet.resourceRevision =
                found.grantSet.resourceRevision;
            result.grantSet.supportedPermissions =
                HumanAccountGrantAdministrationService::
                    supportedGrantPermissions();
            for (const HumanAccountGrantOption& source :
                 HumanAccountGrantAdministrationService::
                     supportedGrantPermissionOptions())
            {
                PublicAccountGrantOption option;
                option.permission = source.permission;
                option.presentationKey = source.presentationKey;
                option.category = source.category;
                result.grantSet.supportedPermissionOptions.push_back(
                    std::move(option));
            }
            result.grantSet.supportedScopeKinds =
                HumanAccountGrantAdministrationService::
                    supportedGrantScopeKinds();
            for (const PermissionGrant& grant :
                 found.grantSet.grants)
            {
                PublicAccountGrantItem item;
                item.permission = grant.permission;
                item.backendId = grant.backendId;
                result.grantSet.grants.push_back(
                    std::move(item));
            }
            return result;
        });

    PublicApiRuntime::instance().registerAccountGrantMutation(
        [this](const PublicAccountGrantMutationRequest& request)
        {
            PublicAccountGrantMutationResult result;
            if (!humanAccountGrantAdministrationService_ ||
                !securityIdentityRepository_)
            {
                return result;
            }

            const std::optional<StoredActorIdentity> actor =
                securityIdentityRepository_->findActor(
                    request.actorRef);
            if (!actor.has_value() ||
                actor->type == ActorType::Anonymous ||
                !actor->active ||
                actor->revoked)
            {
                return result;
            }

            HumanAccountGrantAdministrationContext context;
            context.actorId = request.actorRef;
            context.requestId = request.requestId;
            context.correlationId =
                request.correlationId;

            const HumanAccountGrantAdministrationResult mutated =
                humanAccountGrantAdministrationService_->setGrant(
                    context,
                    request.accountId,
                    request.expectedResourceRevision,
                    request.permission,
                    request.backendId,
                    request.active);

            switch (mutated.status)
            {
                case HumanAccountGrantAdministrationStatus::success:
                    result.status =
                        PublicAccountGrantStatus::ok;
                    break;
                case HumanAccountGrantAdministrationStatus::accountNotFound:
                    result.status =
                        PublicAccountGrantStatus::notFound;
                    return result;
                case HumanAccountGrantAdministrationStatus::invalidGrant:
                    result.status =
                        PublicAccountGrantStatus::invalid;
                    return result;
                case HumanAccountGrantAdministrationStatus::revisionConflict:
                    result.status =
                        PublicAccountGrantStatus::revisionConflict;
                    break;
                case HumanAccountGrantAdministrationStatus::finalAdministrator:
                    result.status =
                        PublicAccountGrantStatus::finalAdministrator;
                    break;
                case HumanAccountGrantAdministrationStatus::accountActorInvalid:
                case HumanAccountGrantAdministrationStatus::entropyUnavailable:
                case HumanAccountGrantAdministrationStatus::storageError:
                    result.status =
                        PublicAccountGrantStatus::unavailable;
                    return result;
            }

            result.grantSet.accountId =
                mutated.grantSet.accountId;
            result.grantSet.actorId =
                mutated.grantSet.actorId;
            result.grantSet.resourceRevision =
                mutated.grantSet.resourceRevision;
            result.grantSet.supportedPermissions =
                HumanAccountGrantAdministrationService::
                    supportedGrantPermissions();
            for (const HumanAccountGrantOption& source :
                 HumanAccountGrantAdministrationService::
                     supportedGrantPermissionOptions())
            {
                PublicAccountGrantOption option;
                option.permission = source.permission;
                option.presentationKey = source.presentationKey;
                option.category = source.category;
                result.grantSet.supportedPermissionOptions.push_back(
                    std::move(option));
            }
            result.grantSet.supportedScopeKinds =
                HumanAccountGrantAdministrationService::
                    supportedGrantScopeKinds();
            for (const PermissionGrant& grant :
                 mutated.grantSet.grants)
            {
                PublicAccountGrantItem item;
                item.permission = grant.permission;
                item.backendId = grant.backendId;
                result.grantSet.grants.push_back(
                    std::move(item));
            }
            return result;
        });

    PublicApiRuntime::instance().registerAccountCreate(
        [this](const PublicAccountCreateRequest& request)
        {
            PublicAccountCreateResult result;
            if (!humanAccountCreationService_ ||
                !securityIdentityRepository_)
            {
                return result;
            }

            const std::optional<StoredActorIdentity> actor =
                securityIdentityRepository_->findActor(
                    request.actorRef);
            if (!actor.has_value() ||
                actor->type == ActorType::Anonymous ||
                !actor->active ||
                actor->revoked)
            {
                return result;
            }

            HumanAccountCreationRequest createRequest;
            createRequest.actorId = request.actorRef;
            createRequest.loginName = request.loginName;
            createRequest.displayName = request.displayName;
            createRequest.password = request.password;
            createRequest.idempotencyKey =
                request.idempotencyKey;
            createRequest.requestId = request.requestId;
            createRequest.correlationId =
                request.correlationId;

            const HumanAccountCreationResult created =
                humanAccountCreationService_->create(
                    std::move(createRequest));

            switch (created.status)
            {
                case HumanAccountCreationStatus::success:
                    result.status =
                        PublicAccountCreateStatus::created;
                    break;
                case HumanAccountCreationStatus::replayed:
                    result.status =
                        PublicAccountCreateStatus::replayed;
                    break;
                case HumanAccountCreationStatus::invalidRequest:
                    result.status =
                        PublicAccountCreateStatus::invalid;
                    return result;
                case HumanAccountCreationStatus::loginConflict:
                    result.status =
                        PublicAccountCreateStatus::loginConflict;
                    return result;
                case HumanAccountCreationStatus::idempotencyConflict:
                    result.status =
                        PublicAccountCreateStatus::idempotencyConflict;
                    return result;
                case HumanAccountCreationStatus::entropyUnavailable:
                case HumanAccountCreationStatus::hashingUnavailable:
                case HumanAccountCreationStatus::storageError:
                    result.status =
                        PublicAccountCreateStatus::unavailable;
                    return result;
            }

            result.account.accountId =
                created.account.accountId;
            result.account.actorId =
                created.account.actorId;
            result.account.displayName =
                created.account.displayName;
            result.account.active =
                created.account.active;
            result.account.resourceRevision =
                "account:" +
                std::to_string(created.account.revision);
            return result;
        });

    PublicApiRuntime::instance().registerAccountMutation(
        [this](const PublicAccountMutationRequest& request)
        {
            PublicAccountMutationResult result;
            if (!humanAccountAdministrationService_ ||
                !securityIdentityRepository_)
            {
                return result;
            }

            const std::optional<StoredActorIdentity> actor =
                securityIdentityRepository_->findActor(
                    request.actorRef);
            if (!actor.has_value() ||
                !actor->active ||
                actor->revoked)
            {
                return result;
            }

            HumanAccountAdministrationContext context;
            context.actorId = request.actorRef;
            context.actorType = actor->type;
            context.requestId = request.requestId;
            context.correlationId =
                request.correlationId;

            const HumanAccountAdministrationResult mutated =
                request.kind ==
                    PublicAccountMutationKind::displayName
                ? humanAccountAdministrationService_->
                    modifyDisplayName(
                        context,
                        request.accountId,
                        request.expectedRevision,
                        request.displayName)
                : humanAccountAdministrationService_->
                    setActive(
                        context,
                        request.accountId,
                        request.expectedRevision,
                        request.active);

            switch (mutated.status)
            {
                case HumanAccountAdministrationStatus::success:
                    result.status =
                        PublicAccountMutationStatus::ok;
                    result.account.accountId =
                        mutated.account.accountId;
                    result.account.actorId =
                        mutated.account.actorId;
                    result.account.displayName =
                        mutated.account.displayName;
                    result.account.active =
                        mutated.account.active;
                    result.account.resourceRevision =
                        "account:" +
                        std::to_string(
                            mutated.account.revision);
                    result.revokedBrowserSessions =
                        mutated.revokedBrowserSessions;
                    return result;

                case HumanAccountAdministrationStatus::invalidRequest:
                    result.status =
                        PublicAccountMutationStatus::invalid;
                    return result;

                case HumanAccountAdministrationStatus::accountNotFound:
                    result.status =
                        PublicAccountMutationStatus::notFound;
                    return result;

                case HumanAccountAdministrationStatus::revisionConflict:
                    result.status =
                        PublicAccountMutationStatus::revisionConflict;
                    return result;

                case HumanAccountAdministrationStatus::finalAdministrator:
                    result.status =
                        PublicAccountMutationStatus::finalAdministrator;
                    return result;

                case HumanAccountAdministrationStatus::accountActorInvalid:
                case HumanAccountAdministrationStatus::entropyUnavailable:
                case HumanAccountAdministrationStatus::storageError:
                    result.status =
                        PublicAccountMutationStatus::unavailable;
                    return result;
            }

            return result;
        });

    browserSessionHttpService_ =
        std::make_unique<BrowserSessionHttpService>(
            *browserSessionIssuanceService_,
            *browserSessionLifecycleService_,
            *accountabilityEventRepository_,
            configuration.browserSessionLifetime,
            configuration.browserSessionConcurrency,
            configuration.browserSessionIdle);

    browserSessionHttpGate_ =
        std::make_unique<BrowserSessionHttpGate>(
            configuration,
            *accountabilityEventRepository_,
            *browserSessionCredentialRepository_,
            *securityPermissionGrantRepository_,
            persistentIdentityResolver_.get(),
            managedBasicAuthenticator_.get(),
            humanPasswordBrowserAuthenticator_.get(),
            humanAccountRepository_.get());

    securityHttpGate_ =
        std::make_unique<SecurityHttpGate>(
            configuration,
            *accountabilityEventRepository_,
            persistentIdentityResolver_.get(),
            managedBasicAuthenticator_.get(),
            browserSessionAuthenticator_.get());
    securityReady_ = true;
}

TestHttpServer::~TestHttpServer()
{
    PublicApiRuntime::instance().resetDevicePairingLookup();
    PublicApiRuntime::instance().resetDevicePairingCreate();
    PublicApiRuntime::instance().resetAccountCredentialMutation();
    PublicApiRuntime::instance().resetAccountCredentialItemLookup();
    PublicApiRuntime::instance().resetAccountSessionMutation();
    PublicApiRuntime::instance().resetAccountSessionItemLookup();
    PublicApiRuntime::instance().resetAccountSessionLookup();
    PublicApiRuntime::instance().resetAccountCredentialLookup();
    PublicApiRuntime::instance().resetAccountGrantMutation();
    PublicApiRuntime::instance().resetAccountGrantLookup();
    PublicApiRuntime::instance().resetAccountCreate();
    PublicApiRuntime::instance().resetAccountMutation();
}

HttpServerResponse TestHttpServer::finalizeResponse(
    const RequestSecurityContext& context,
    HttpServerResponse response) const
{
    if (securityHttpGate_)
    {
        securityHttpGate_->decorateResponse(
            context,
            response);
    }

    return response;
}

HttpServerResponse TestHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    if (!securityReady_ ||
        !securityHttpGate_ ||
        !browserSessionHttpGate_ ||
        !browserSessionHttpService_ ||
        !firstAdminClaimHttpService_)
    {
        HttpServerResponse response;
        response.statusCode = 503;
        response.headers["Content-Type"] =
            "application/json";
        response.headers["Cache-Control"] =
            "no-store";
        response.body =
            "{\"error\":{\"code\":\"security_runtime_unavailable\","
            "\"message\":\"The security runtime is unavailable\"}}";
        return response;
    }

    if (request.method == "GET" &&
        isFrontendPath(request.path))
    {
        return serveFrontendPath(request.path);
    }

    if (request.method == "GET" &&
        isChannelLogoPath(request.path))
    {
        return makeChannelLogoResponse(request.path);
    }

    if (firstAdminClaimHttpService_->handles(request))
    {
        return firstAdminClaimHttpService_->handle(request);
    }

    if (browserSessionHttpGate_->handles(request))
    {
        const BrowserSessionGateDecision browserGate =
            browserSessionHttpGate_->evaluate(request);
        if (!browserGate.allowed)
        {
            return browserGate.rejection;
        }

        HttpServerResponse response = browserGate.login
            ? browserSessionHttpService_->login(browserGate.context)
            : browserSessionHttpService_->logout(browserGate.context);
        return finalizeResponse(
            browserGate.context,
            std::move(response));
    }

    const HttpServerRequest securityRequest =
        ContinueWatchingSecurityRequest::forAuthorization(
            SeriesArtworkSettingsSecurityRequest::forAuthorization(request));
    SecurityGateDecision gate;
    {
        const HttpServerRequest& request = securityRequest;
        gate = securityHttpGate_->evaluate(request);
    }

    if (!gate.allowed)
    {
        return gate.rejection;
    }

    if (request.method == "GET" &&
        pathWithoutQuery(request.path) == BrowserSessionCurrentPath)
    {
        BrowserSessionCsrfRecoveryService recovery(
            *browserSessionCredentialRepository_);
        return finalizeResponse(
            gate.context,
            recovery.recover(gate.context));
    }

    ApiResponse apiResponse;

    if (request.method == "GET")
    {
        apiResponse =
            apiRouter_.handleClientGet(
                request.path,
                gate.context.actor.actorId,
                hbbtvClientContext(gate.context),
                gate.context.requestId,
                gate.context.correlationId,
                requestHeaderValue(
                    request,
                    "If-None-Match"),
                gate.authorizationDecision.backendId,
                gate.authorizedBackendIds,
                requestHeaderValue(
                    request,
                    "X-VDR-Suite-Pairing-Token"));
    }
    else if (request.method == "POST")
    {
        apiResponse =
            apiRouter_.handleClientPost(
                request.path,
                request.body,
                gate.context.actor.actorId,
                hbbtvClientContext(gate.context),
                gate.context.correlationId,
                gate.context.requestId,
                requestHeaderValue(
                    request,
                    "If-Match"),
                requestHeaderValue(
                    request,
                    "Idempotency-Key"),
                requestHeaderValue(
                    request,
                    "Content-Type"),
                gate.authorizationDecision.backendId);
    }
    else
    {
        apiResponse =
            apiRouter_.handleClientUnsupportedMethod(
                request.method,
                request.path,
                gate.context.requestId,
                gate.context.correlationId);
    }

    if (gate.protectedMutation &&
        !securityHttpGate_->appendProtectedMutationOutcome(
            gate,
            apiResponse.statusCode))
    {
        return securityHttpGate_->
            outcomeAccountabilityUnavailableResponse(
                gate.context);
    }

    return finalizeResponse(
        gate.context,
        mapApiResponse(
            apiResponse.statusCode,
            apiResponse.contentType,
            apiResponse.body,
            apiResponse.headers));
}

HttpServerResponse TestHttpServer::mapApiResponse(
    int statusCode,
    const std::string& contentType,
    const std::string& body,
    const std::map<std::string, std::string>& headers) const
{
    HttpServerResponse response;
    response.statusCode = statusCode;
    response.headers = headers;
    response.headers["Content-Type"] = contentType;
    response.body = body;
    return response;
}
