#include "TestHttpServer.h"

#include "ApiRouter.h"
#include "DeviceLiveEventHub.h"
#include "BrowserSessionCsrfRecoveryService.h"
#include "ContinueWatchingSecurityRequest.h"
#include "FirstAdminClaimHttpService.h"
#include "SecurityConfiguration.h"
#include "RestQueryParameters.h"
#include "SeriesArtworkSettingsSecurityRequest.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <limits>
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

// P1: exact admin path. Never treat pairing approval as an Account owner.
bool devicePlaybackAccountPath(const std::string& path, std::string& deviceId)
{
    const std::string prefix = "/api/v1/devices/";
    const std::string suffix = "/playback-account";
    deviceId.clear();
    if (path.compare(0, prefix.size(), prefix) != 0 ||
        path.size() <= prefix.size() + suffix.size() ||
        path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
        return false;
    deviceId = path.substr(prefix.size(),
        path.size() - prefix.size() - suffix.size());
    return !deviceId.empty() && deviceId.size() <= 128U &&
        std::all_of(deviceId.begin(),deviceId.end(),[](unsigned char c) {
            return std::isalnum(c) || c == '-' || c == '_' || c == '.';
        });
}

// Refuse unknown/duplicate fields and JSON type coercion. Binding mutation
// is deliberately tiny, uses no native identity or privileged client claim.
bool parsePlaybackAccountBinding(const std::string& body,
                                 std::string& accountId,
                                 std::int64_t& expectedRevision)
{
    if (body.size() > 512U) return false;
    static const std::regex first(
        R"p1(^\s*\{\s*"accountId"\s*:\s*"([A-Za-z0-9_.-]{0,128})"\s*,\s*"expectedRevision"\s*:\s*(0|[1-9][0-9]{0,18})\s*\}\s*$)p1");
    static const std::regex second(
        R"p1(^\s*\{\s*"expectedRevision"\s*:\s*(0|[1-9][0-9]{0,18})\s*,\s*"accountId"\s*:\s*"([A-Za-z0-9_.-]{0,128})"\s*\}\s*$)p1");
    std::smatch match;
    bool reversed = false;
    if (!std::regex_match(body,match,first)) {
        if (!std::regex_match(body,match,second)) return false;
        reversed = true;
    }
    accountId = match[reversed ? 2 : 1].str();
    try {
        expectedRevision = std::stoll(match[reversed ? 1 : 2].str());
    } catch (...) {
        return false;
    }
    return expectedRevision >= 0;
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

PublicDevicePairingAdministrationStatus
publicDevicePairingAdministrationStatus(
    DevicePairingAdministrationStatus status)
{
    switch (status)
    {
        case DevicePairingAdministrationStatus::ok:
            return PublicDevicePairingAdministrationStatus::ok;
        case DevicePairingAdministrationStatus::invalidRequest:
            return PublicDevicePairingAdministrationStatus::invalid;
        case DevicePairingAdministrationStatus::notFound:
            return PublicDevicePairingAdministrationStatus::notFound;
        case DevicePairingAdministrationStatus::expired:
            return PublicDevicePairingAdministrationStatus::expired;
        case DevicePairingAdministrationStatus::revisionConflict:
            return PublicDevicePairingAdministrationStatus::revisionConflict;
        case DevicePairingAdministrationStatus::stateConflict:
            return PublicDevicePairingAdministrationStatus::stateConflict;
        case DevicePairingAdministrationStatus::entropyUnavailable:
        case DevicePairingAdministrationStatus::storageError:
            return PublicDevicePairingAdministrationStatus::unavailable;
    }
    return PublicDevicePairingAdministrationStatus::unavailable;
}

void copyDevicePairingAdministrativeResource(
    const DevicePairingAdministrativeResource& source,
    PublicDevicePairingAdministrativeResource& target)
{
    target.resource.pairingRequestId =
        source.resource.pairingRequestId;
    target.resource.client.displayName =
        source.resource.client.displayName;
    target.resource.client.clientKind =
        source.resource.client.clientKind;
    target.resource.client.appVersion =
        source.resource.client.appVersion;
    target.resource.state =
        source.resource.state;
    target.resource.expiresAt =
        source.resource.expiresAt;
    target.resource.pollIntervalSeconds =
        source.resource.pollIntervalSeconds;
    target.resourceRevision =
        source.resourceRevision;
    target.decidedByActorId =
        source.decidedByActorId;
    target.decidedAt =
        source.decidedAt;
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

TestHttpServer::TestHttpServer(
    ApiRouter& apiRouter,
    HumanAccountGrantAdministrationService::AddonGrantEligibility
        addonGrantEligibility,
    AddonActivationMutation addonActivationMutation,
    AddonActivationLookup addonActivationLookup,
    AddonSourceMutation addonSourceMutation,
    AddonSourceLookup addonSourceLookup)
    : TestHttpServer(apiRouter, nullptr, std::move(addonGrantEligibility),
                     std::move(addonActivationMutation),
                     std::move(addonActivationLookup),
                     std::move(addonSourceMutation),
                     std::move(addonSourceLookup)) {}

TestHttpServer::TestHttpServer(
    ApiRouter& apiRouter,
    std::shared_ptr<DeviceLiveEventHub> eventHub,
    HumanAccountGrantAdministrationService::AddonGrantEligibility
        addonGrantEligibility,
    AddonActivationMutation addonActivationMutation,
    AddonActivationLookup addonActivationLookup,
    AddonSourceMutation addonSourceMutation,
    AddonSourceLookup addonSourceLookup)
    : apiRouter_(apiRouter),
      deviceEventHub_(std::move(eventHub)),
      addonActivationMutation_(std::move(addonActivationMutation)),
      addonActivationLookup_(std::move(addonActivationLookup)),
      addonSourceMutation_(std::move(addonSourceMutation)),
      addonSourceLookup_(std::move(addonSourceLookup)),
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

    deviceCredentialVerifierRepository_ =
        std::make_unique<DeviceCredentialVerifierRepository>(
            *securityDatabase_);
    if (!deviceCredentialVerifierRepository_->ensureSchema())
        return;


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
            *firstAdminClaimService_, firstAdminBootstrapRepository_.get());

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
            *accountabilityEventRepository_,
            HumanAccountGrantAdministrationService::EntropySource{},
            HumanAccountGrantAdministrationService::Clock{},
            std::move(addonGrantEligibility));

    deviceGrantAdministrationService_ =
        std::make_unique<DeviceGrantAdministrationService>(
            *securityDatabase_, *securityIdentityRepository_,
            *deviceCredentialVerifierRepository_,
            *securityPermissionGrantRepository_,
            *accountabilityEventRepository_);

    devicePlaybackAccountBindingService_ =
        std::make_unique<DevicePlaybackAccountBindingService>(
            *securityDatabase_, *securityIdentityRepository_,
            *humanAccountRepository_, *securityPermissionGrantRepository_,
            *accountabilityEventRepository_);
    if (!devicePlaybackAccountBindingService_->ensureSchema())
        return;

    deviceLifecycleAdministrationService_ =
        std::make_unique<DeviceLifecycleAdministrationService>(
            *securityDatabase_, *securityIdentityRepository_,
            *deviceCredentialVerifierRepository_,
            *accountabilityEventRepository_);

    deviceCredentialRotationService_ =
        std::make_unique<DeviceCredentialRotationService>(
            *securityDatabase_, *securityIdentityRepository_,
            *deviceCredentialVerifierRepository_,
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
                case DevicePairingPollStatus::consumed:
                    result.status =
                        PublicDevicePairingLookupStatus::consumed;
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

    PublicApiRuntime::instance().registerDeviceCredentialIssue(
        [this](const PublicDeviceCredentialIssueRequest& request)
        {
            PublicDeviceCredentialIssueResult result;
            if (!devicePairingRequestService_ ||
                !securityIdentityProvisioningRepository_ ||
                !deviceCredentialVerifierRepository_)
                return result;

            DevicePairingCredentialIssueRequest serviceRequest;
            serviceRequest.pairingRequestId = request.pairingRequestId;
            serviceRequest.pairingToken = request.pairingToken;
            serviceRequest.requestId = request.requestId;
            serviceRequest.correlationId = request.correlationId;
            auto issued = devicePairingRequestService_->issueDeviceCredential(
                serviceRequest, *securityIdentityProvisioningRepository_,
                *deviceCredentialVerifierRepository_);
            switch (issued.status)
            {
                case DevicePairingCredentialIssueStatus::issued:
                    result.status = PublicDeviceCredentialIssueStatus::issued;
                    break;
                case DevicePairingCredentialIssueStatus::invalidRequest:
                    result.status = PublicDeviceCredentialIssueStatus::invalid;
                    break;
                case DevicePairingCredentialIssueStatus::notFound:
                    result.status = PublicDeviceCredentialIssueStatus::notFound;
                    break;
                case DevicePairingCredentialIssueStatus::unauthorized:
                    result.status = PublicDeviceCredentialIssueStatus::unauthorized;
                    break;
                case DevicePairingCredentialIssueStatus::notApproved:
                    result.status = PublicDeviceCredentialIssueStatus::notApproved;
                    break;
                case DevicePairingCredentialIssueStatus::expired:
                    result.status = PublicDeviceCredentialIssueStatus::expired;
                    break;
                case DevicePairingCredentialIssueStatus::consumed:
                    result.status = PublicDeviceCredentialIssueStatus::consumed;
                    break;
                case DevicePairingCredentialIssueStatus::entropyUnavailable:
                case DevicePairingCredentialIssueStatus::hashingUnavailable:
                case DevicePairingCredentialIssueStatus::storageError:
                    result.status = PublicDeviceCredentialIssueStatus::unavailable;
                    break;
            }
            if (result.status == PublicDeviceCredentialIssueStatus::issued &&
                issued.credential.has_value())
            {
                result.actorId = issued.credential->actorId;
                result.deviceId = issued.credential->deviceId;
                result.credentialId = issued.credential->credentialId;
                result.credentialSecret =
                    std::move(issued.credential->credentialSecret);
                issued.credential->clearSecret();
            }
            return result;
        });

    PublicApiRuntime::instance().
        registerDevicePairingAdministrationCollectionLookup(
            [this](
                const PublicDevicePairingAdministrationCollectionRequest&
                    request)
            {
                PublicDevicePairingAdministrationCollectionResult result;
                if (!devicePairingRequestService_)
                    return result;

                const DevicePairingAdministrationCollectionResult found =
                    devicePairingRequestService_->
                        listPendingForAdministration(
                            request.afterPairingRequestId,
                            request.limit);
                result.status =
                    publicDevicePairingAdministrationStatus(
                        found.status);
                if (found.status !=
                    DevicePairingAdministrationStatus::ok)
                {
                    return result;
                }

                result.hasMore = found.hasMore;
                for (const auto& source : found.requests)
                {
                    PublicDevicePairingAdministrativeResource target;
                    copyDevicePairingAdministrativeResource(
                        source,
                        target);
                    result.requests.push_back(
                        std::move(target));
                }
                return result;
            });

    PublicApiRuntime::instance().
        registerDevicePairingAdministrationLookup(
            [this](const std::string& pairingRequestId)
            {
                PublicDevicePairingAdministrationLookupResult result;
                if (!devicePairingRequestService_)
                    return result;

                const DevicePairingAdministrationReadResult found =
                    devicePairingRequestService_->
                        readForAdministration(
                            pairingRequestId);
                result.status =
                    publicDevicePairingAdministrationStatus(
                        found.status);
                if (found.status ==
                    DevicePairingAdministrationStatus::ok)
                {
                    copyDevicePairingAdministrativeResource(
                        found.request,
                        result.request);
                }
                return result;
            });

    PublicApiRuntime::instance().registerDevicePairingDecision(
        [this](const PublicDevicePairingDecisionRequest& request)
        {
            PublicDevicePairingDecisionResult result;
            if (!devicePairingRequestService_ ||
                !securityIdentityRepository_)
            {
                return result;
            }

            const std::optional<StoredActorIdentity> actor =
                securityIdentityRepository_->findActor(
                    request.actorRef);
            if (!actor.has_value() ||
                actor->type != ActorType::User ||
                !actor->active ||
                actor->revoked)
            {
                return result;
            }

            DevicePairingDecisionRequest serviceRequest;
            serviceRequest.context.actorId =
                request.actorRef;
            serviceRequest.context.actorType =
                "user";
            serviceRequest.context.requestId =
                request.requestId;
            serviceRequest.context.correlationId =
                request.correlationId;
            serviceRequest.pairingRequestId =
                request.pairingRequestId;
            serviceRequest.expectedResourceRevision =
                request.expectedResourceRevision;
            serviceRequest.decision =
                request.decision;

            const DevicePairingDecisionResult decided =
                devicePairingRequestService_->decide(
                    serviceRequest);
            result.status =
                publicDevicePairingAdministrationStatus(
                    decided.status);
            if (decided.status ==
                    DevicePairingAdministrationStatus::ok ||
                decided.status ==
                    DevicePairingAdministrationStatus::revisionConflict ||
                decided.status ==
                    DevicePairingAdministrationStatus::stateConflict)
            {
                copyDevicePairingAdministrativeResource(
                    decided.request,
                    result.request);
            }
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

    PublicApiRuntime::instance().registerDeviceLifecycleLookup(
        [this](const std::string& deviceId, const std::string& credentialId)
        {
            PublicDeviceLifecycleResult response;
            if (!deviceLifecycleAdministrationService_)
                return response;
            const auto result = deviceLifecycleAdministrationService_->read(
                credentialId.empty() ? DeviceLifecycleTarget::Device
                                     : DeviceLifecycleTarget::Credential,
                deviceId, credentialId);
            switch (result.status)
            {
                case DeviceLifecycleStatus::ok:
                    response.status = PublicDeviceLifecycleStatus::ok;
                    break;
                case DeviceLifecycleStatus::invalid:
                    response.status = PublicDeviceLifecycleStatus::invalid;
                    return response;
                case DeviceLifecycleStatus::notFound:
                    response.status = PublicDeviceLifecycleStatus::notFound;
                    return response;
                case DeviceLifecycleStatus::revisionConflict:
                case DeviceLifecycleStatus::unavailable:
                    return response;
            }
            response.resource.deviceId = result.resource.deviceId;
            response.resource.actorId = result.resource.actorId;
            response.resource.credentialId = result.resource.credentialId;
            response.resource.active = result.resource.active;
            response.resource.revoked = result.resource.revoked;
            response.resource.resourceRevision = result.resource.resourceRevision;
            return response;
        });

    PublicApiRuntime::instance().registerDeviceLifecycleMutation(
        [this](const PublicDeviceLifecycleMutationRequest& request)
        {
            PublicDeviceLifecycleResult response;
            if (!deviceLifecycleAdministrationService_ ||
                !securityIdentityRepository_)
                return response;
            const auto administrator =
                securityIdentityRepository_->findActor(request.actorRef);
            if (!administrator.has_value() ||
                administrator->type != ActorType::User ||
                !administrator->active || administrator->revoked)
                return response;
            const auto result = deviceLifecycleAdministrationService_->revoke(
                {request.actorRef, request.requestId, request.correlationId},
                request.credentialId.empty() ? DeviceLifecycleTarget::Device
                    : DeviceLifecycleTarget::Credential,
                request.deviceId, request.credentialId,
                request.expectedResourceRevision);
            switch (result.status)
            {
                case DeviceLifecycleStatus::ok:
                    response.status = PublicDeviceLifecycleStatus::ok;
                    break;
                case DeviceLifecycleStatus::invalid:
                    response.status = PublicDeviceLifecycleStatus::invalid;
                    return response;
                case DeviceLifecycleStatus::notFound:
                    response.status = PublicDeviceLifecycleStatus::notFound;
                    return response;
                case DeviceLifecycleStatus::revisionConflict:
                    response.status =
                        PublicDeviceLifecycleStatus::revisionConflict;
                    return response;
                case DeviceLifecycleStatus::unavailable:
                    return response;
            }
            response.resource.deviceId = result.resource.deviceId;
            response.resource.actorId = result.resource.actorId;
            response.resource.credentialId = result.resource.credentialId;
            response.resource.active = result.resource.active;
            response.resource.revoked = result.resource.revoked;
            response.resource.resourceRevision = result.resource.resourceRevision;
            return response;
        });

    PublicApiRuntime::instance().registerDeviceCredentialRotation(
        [this](const PublicDeviceCredentialRotationRequest& input)
        {
            PublicDeviceCredentialRotationResult response;
            if (!deviceCredentialRotationService_ ||
                !securityIdentityRepository_)
                return response;
            const auto admin = securityIdentityRepository_->findActor(
                input.actorRef);
            if (!admin || admin->type != ActorType::User ||
                !admin->active || admin->revoked)
                return response;
            DeviceCredentialRotationRequest request;
            request.administratorActorId = input.actorRef;
            request.deviceId = input.deviceId;
            request.previousCredentialId = input.credentialId;
            request.expectedResourceRevision = input.expectedResourceRevision;
            request.requestId = input.requestId;
            request.correlationId = input.correlationId;
            auto result = deviceCredentialRotationService_->rotate(request);
            switch (result.status)
            {
                case DeviceCredentialRotationStatus::rotated:
                    response.status =
                        PublicDeviceCredentialRotationStatus::rotated;
                    break;
                case DeviceCredentialRotationStatus::invalid:
                    response.status = PublicDeviceCredentialRotationStatus::invalid;
                    break;
                case DeviceCredentialRotationStatus::notFound:
                    response.status = PublicDeviceCredentialRotationStatus::notFound;
                    break;
                case DeviceCredentialRotationStatus::stateConflict:
                    response.status = PublicDeviceCredentialRotationStatus::stateConflict;
                    break;
                case DeviceCredentialRotationStatus::revisionConflict:
                    response.status = PublicDeviceCredentialRotationStatus::revisionConflict;
                    break;
                case DeviceCredentialRotationStatus::unavailable:
                    return response;
            }
            if (result.issued)
            {
                response.actorId = result.issued->actorId;
                response.deviceId = result.issued->deviceId;
                response.credentialId = result.issued->credentialId;
                response.credentialSecret = result.issued->credentialSecret;
                result.issued->clearSecret();
            }
            return response;
        });

    PublicApiRuntime::instance().registerDeviceCollectionLookup(
        [this](const PublicDeviceCollectionRequest& request)
        {
            PublicDeviceCollectionResult result;
            if (!securityIdentityRepository_ || request.limit < 1U ||
                request.limit > 100U) return result;
            const auto devices = securityIdentityRepository_->listDevices(
                request.afterDeviceId, request.limit + 1U);
            if (!devices.has_value()) return result;
            result.available = true;
            result.hasMore = devices->size() > request.limit;
            for (std::size_t i = 0U;
                 i < devices->size() && i < request.limit; ++i)
            {
                const auto& device = (*devices)[i];
                result.items.push_back(PublicDeviceCollectionItem{
                    device.deviceId, device.actorId, device.displayName,
                    device.active, device.revoked});
            }
            return result;
        });

    PublicApiRuntime::instance().registerDeviceGrantLookup(
        [this](const std::string& deviceId)
        {
            PublicDeviceGrantLookupResult response;
            if (!deviceGrantAdministrationService_)
                return response;
            const DeviceGrantAdministrationResult result =
                deviceGrantAdministrationService_->read(deviceId);
            switch (result.status)
            {
                case DeviceGrantAdministrationStatus::ok:
                    response.status = PublicDeviceGrantStatus::ok;
                    break;
                case DeviceGrantAdministrationStatus::invalid:
                    response.status = PublicDeviceGrantStatus::invalid;
                    return response;
                case DeviceGrantAdministrationStatus::notFound:
                    response.status = PublicDeviceGrantStatus::notFound;
                    return response;
                case DeviceGrantAdministrationStatus::revisionConflict:
                case DeviceGrantAdministrationStatus::unavailable:
                    return response;
            }
            response.grantSet.deviceId = result.grantSet.deviceId;
            response.grantSet.actorId = result.grantSet.actorId;
            response.grantSet.resourceRevision = result.grantSet.revision;
            for (const PermissionGrant& grant : result.grantSet.grants)
                response.grantSet.grants.push_back(
                    PublicAccountGrantItem{grant.permission, grant.backendId});
            return response;
        });

    PublicApiRuntime::instance().registerDeviceGrantMutation(
        [this](const PublicDeviceGrantMutationRequest& request)
        {
            PublicDeviceGrantMutationResult response;
            if (!deviceGrantAdministrationService_ ||
                !securityIdentityRepository_)
                return response;
            const auto administrator =
                securityIdentityRepository_->findActor(request.actorRef);
            if (!administrator.has_value() ||
                administrator->type != ActorType::User ||
                !administrator->active || administrator->revoked)
                return response;

            const DeviceGrantAdministrationResult result =
                deviceGrantAdministrationService_->setGrant(
                    {request.actorRef, request.requestId,
                     request.correlationId},
                    request.deviceId, request.expectedResourceRevision,
                    request.permission, request.backendId, request.active);
            switch (result.status)
            {
                case DeviceGrantAdministrationStatus::ok:
                    response.status = PublicDeviceGrantStatus::ok;
                    break;
                case DeviceGrantAdministrationStatus::invalid:
                    response.status = PublicDeviceGrantStatus::invalid;
                    return response;
                case DeviceGrantAdministrationStatus::notFound:
                    response.status = PublicDeviceGrantStatus::notFound;
                    return response;
                case DeviceGrantAdministrationStatus::revisionConflict:
                    response.status = PublicDeviceGrantStatus::revisionConflict;
                    return response;
                case DeviceGrantAdministrationStatus::unavailable:
                    return response;
            }
            response.grantSet.deviceId = result.grantSet.deviceId;
            response.grantSet.actorId = result.grantSet.actorId;
            response.grantSet.resourceRevision = result.grantSet.revision;
            for (const PermissionGrant& grant : result.grantSet.grants)
                response.grantSet.grants.push_back(
                    PublicAccountGrantItem{grant.permission, grant.backendId});
            return response;
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

    deviceCredentialAuthenticator_ =
        std::make_unique<DeviceCredentialAuthenticator>(
            *deviceCredentialVerifierRepository_,
            *securityIdentityRepository_,
            *securityPermissionGrantRepository_);

    securityHttpGate_ =
        std::make_unique<SecurityHttpGate>(
            configuration,
            *accountabilityEventRepository_,
            persistentIdentityResolver_.get(),
            managedBasicAuthenticator_.get(),
            browserSessionAuthenticator_.get(),
            deviceCredentialAuthenticator_.get());
    securityReady_ = true;
}

TestHttpServer::~TestHttpServer()
{
    PublicApiRuntime::instance().resetDevicePairingDecision();
    PublicApiRuntime::instance().
        resetDevicePairingAdministrationLookup();
    PublicApiRuntime::instance().
        resetDevicePairingAdministrationCollectionLookup();
    PublicApiRuntime::instance().resetDeviceCredentialIssue();
    PublicApiRuntime::instance().resetDevicePairingLookup();
    PublicApiRuntime::instance().resetDevicePairingCreate();
    PublicApiRuntime::instance().resetAccountCredentialMutation();
    PublicApiRuntime::instance().resetAccountCredentialItemLookup();
    PublicApiRuntime::instance().resetAccountSessionMutation();
    PublicApiRuntime::instance().resetAccountSessionItemLookup();
    PublicApiRuntime::instance().resetAccountSessionLookup();
    PublicApiRuntime::instance().resetAccountCredentialLookup();
    PublicApiRuntime::instance().resetDeviceLifecycleMutation();
    PublicApiRuntime::instance().resetDeviceCredentialRotation();
    PublicApiRuntime::instance().resetDeviceLifecycleLookup();
    PublicApiRuntime::instance().resetDeviceGrantMutation();
    PublicApiRuntime::instance().resetDeviceGrantLookup();
    PublicApiRuntime::instance().resetDeviceCollectionLookup();
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

        // Cookies without Secure are confined to direct, HTTP-only clients on
        // a private LAN. Loopback reverse proxies keep Secure by default.
        // Never infer transport security from client-controlled proxy headers.
        const std::string host = requestHeaderValue(request, "Host");
        const std::string origin = requestHeaderValue(request, "Origin");
        const bool directHttpLan = request.peerIsDirectPrivateLan &&
            !host.empty() && origin == "http://" + host &&
            requestHeaderValue(request, "Forwarded").empty() &&
            requestHeaderValue(request, "X-Forwarded-For").empty() &&
            requestHeaderValue(request, "X-Forwarded-Host").empty() &&
            requestHeaderValue(request, "X-Forwarded-Proto").empty();
        HttpServerResponse response = browserGate.login
            ? browserSessionHttpService_->login(browserGate.context, directHttpLan)
            : browserSessionHttpService_->logout(browserGate.context, directHttpLan);
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

    // M2: serve the existing EPG Artwork repository through the same
    // exact-backend epg.view SecurityHttpGate used for EPG detail.
    // No new provider, arbitrary path or URL input can reach this callback.
    const bool isEpgDirectArtwork =
        pathWithoutQuery(request.path) == "/api/v1/epg/artwork";
    const bool isEpgPreferredArtwork =
        pathWithoutQuery(request.path) == "/api/v1/epg/preferred-artwork";
    if (request.method == "GET" &&
        (isEpgDirectArtwork || isEpgPreferredArtwork))
    {
        const auto queryAt = request.path.find('?');
        const std::string rawQuery = queryAt == std::string::npos
            ? std::string() : request.path.substr(queryAt + 1U);
        const RestQueryParameters query = RestQueryParameters::parse(rawQuery);
        // RestQueryParameters intentionally collapses duplicates for legacy
        // callers. Public-v1 artwork must not accept ambiguous identities.
        std::size_t backendCount = 0U;
        std::size_t channelCount = 0U;
        std::size_t eventCount = 0U;
        bool canonicalQueryKeys = true;
        for (std::size_t start = 0U; start < rawQuery.size();)
        {
            const std::size_t end = rawQuery.find('&', start);
            const std::string item = rawQuery.substr(start,
                end == std::string::npos ? std::string::npos : end - start);
            const std::size_t equals = item.find('=');
            const std::string key = item.substr(0, equals);
            if (equals == std::string::npos)
                canonicalQueryKeys = false;
            else if (key == "backendId") ++backendCount;
            else if (key == "channelId") ++channelCount;
            else if (key == "eventId") ++eventCount;
            else canonicalQueryKeys = false;
            if (end == std::string::npos) break;
            start = end + 1U;
        }
        const bool exactQuery = canonicalQueryKeys &&
            backendCount == 1U && channelCount == 1U && eventCount == 1U;
        const std::string backend = query.get("backendId");
        const std::string channel = query.get("channelId");
        const std::string event = query.get("eventId");
        const auto validIdentity = [](const std::string& value) {
            return !value.empty() && value.size() <= 256U &&
                std::all_of(value.begin(), value.end(),
                    [](unsigned char ch) {
                        return (ch >= 'A' && ch <= 'Z') ||
                               (ch >= 'a' && ch <= 'z') ||
                               (ch >= '0' && ch <= '9') ||
                               ch == '-' || ch == '_' || ch == '.' ||
                               ch == ':';
                    });
        };
        ApiResponse response;
        if (!exactQuery ||
            gate.authorizedBackendIds.size() != 1U ||
            backend != gate.authorizedBackendIds.front() ||
            !validIdentity(channel) || !validIdentity(event))
        {
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = "{\"error\":\"invalid_epg_artwork_scope\"}";
        }
        else
        {
            response = isEpgPreferredArtwork
                ? apiRouter_.getEpgPreferredArtwork(backend, channel, event)
                : apiRouter_.getEpgArtwork(backend, channel, event);
            if (response.statusCode == 200 &&
                response.contentType != "image/png" &&
                response.contentType != "image/jpeg" &&
                response.contentType != "image/webp")
            {
                response.statusCode = 503;
                response.contentType = "application/json";
                response.body = "{\"error\":\"invalid_epg_artwork_type\"}";
            }
        }
        response.headers["Cache-Control"] = response.statusCode == 200
            ? "private, max-age=300" : "no-store";
        response.headers["X-Content-Type-Options"] = "nosniff";
        return finalizeResponse(gate.context,
            mapApiResponse(response.statusCode, response.contentType,
                response.body, response.headers));
    }

    if (request.method == "GET" &&
        pathWithoutQuery(request.path) == "/api/v1/events")
    {
        if (!gate.deviceAuthenticated ||
            gate.authorizedBackendIds.size() != 1U ||
            gate.deviceEventDomains.empty() || !deviceEventHub_)
        {
            HttpServerResponse unavailable;
            unavailable.statusCode = 503;
            unavailable.headers["Content-Type"] = "application/json";
            unavailable.headers["Cache-Control"] = "no-store";
            unavailable.body = "{\"error\":\"device_event_stream_unavailable\"}";
            return finalizeResponse(gate.context, std::move(unavailable));
        }
        HttpServerResponse response;
        response.deviceEventSubscription = deviceEventHub_->subscribe(
            gate.authorizedBackendIds.front(), gate.deviceEventDomains);
        if (!response.deviceEventSubscription)
        {
            response.statusCode = 503;
            response.headers["Content-Type"] = "application/json";
            response.headers["Cache-Control"] = "no-store";
            response.body = "{\"error\":\"device_event_capacity_exhausted\"}";
        }
        else
        {
            response.statusCode = 200;
            response.headers["Content-Type"] = "text/event-stream";
            response.headers["Cache-Control"] = "no-store";
            response.headers["X-Accel-Buffering"] = "no";
        }
        return finalizeResponse(gate.context, std::move(response));
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
    std::string playbackBindingDeviceId;
    if (devicePlaybackAccountPath(pathWithoutQuery(request.path),
                                  playbackBindingDeviceId) &&
        (request.method == "GET" || request.method == "POST"))
    {
        apiResponse.contentType = "application/json";
        apiResponse.headers["Cache-Control"] = "no-store";
        apiResponse.headers["X-Content-Type-Options"] = "nosniff";
        if (!devicePlaybackAccountBindingService_) {
            apiResponse.statusCode = 503;
            apiResponse.body = R"({"error":{"code":"playback_binding_unavailable"}})";
        } else if (request.path != pathWithoutQuery(request.path)) {
            apiResponse.statusCode = 400;
            apiResponse.body = R"({"error":{"code":"playback_binding_query_forbidden"}})";
        } else if (request.method == "GET") {
            if (!request.body.empty()) {
                apiResponse.statusCode = 400;
                apiResponse.body = R"({"error":{"code":"playback_binding_get_body_forbidden"}})";
            } else {
                const auto found = devicePlaybackAccountBindingService_->read(
                    playbackBindingDeviceId);
                using S = DevicePlaybackAccountStatus;
                apiResponse.statusCode = found.status == S::ok ? 200 :
                    found.status == S::invalid ? 400 :
                    found.status == S::notFound ? 404 : 503;
                if (found.status == S::ok) {
                    apiResponse.body = "{\"deviceId\":\"" +
                        found.binding.deviceId + "\",\"accountId\":";
                    if (found.binding.accountId.empty())
                        apiResponse.body += "null";
                    else apiResponse.body += "\"" + found.binding.accountId + "\"";
                    apiResponse.body += ",\"revision\":" +
                        std::to_string(found.binding.revision) + "}";
                } else {
                    apiResponse.body = R"({"error":{"code":"playback_binding_unavailable_or_missing"}})";
                }
            }
        } else {
            std::string accountId;
            std::int64_t expectedRevision = -1;
            const std::string contentType =
                requestHeaderValue(request, "Content-Type");
            if (contentType != "application/json" ||
                !parsePlaybackAccountBinding(request.body, accountId,
                                             expectedRevision)) {
                apiResponse.statusCode = 400;
                apiResponse.body = R"({"error":{"code":"playback_binding_invalid_payload"}})";
            } else {
                const auto updated = devicePlaybackAccountBindingService_->set(
                    gate.context.actor.actorId,
                    playbackBindingDeviceId, accountId,
                    expectedRevision, gate.context.requestId);
                using S = DevicePlaybackAccountStatus;
                apiResponse.statusCode = updated.status == S::ok ? 200 :
                    updated.status == S::invalid ? 400 :
                    updated.status == S::notFound ? 404 :
                    updated.status == S::revisionConflict ? 409 : 503;
                if (updated.status == S::ok) {
                    apiResponse.body = "{\"deviceId\":\"" +
                        updated.binding.deviceId + "\",\"accountId\":";
                    if (updated.binding.accountId.empty())
                        apiResponse.body += "null";
                    else apiResponse.body += "\"" + updated.binding.accountId + "\"";
                    apiResponse.body += ",\"revision\":" +
                        std::to_string(updated.binding.revision) + "}";
                } else {
                    apiResponse.body = R"({"error":{"code":"playback_binding_mutation_rejected"}})";
                }
            }
        }
    }
    else if (request.method == "GET")
    {
        if (pathWithoutQuery(request.path) ==
                "/api/v1/addons/rectools/activation")
        {
            apiResponse = addonActivationLookup_
                ? addonActivationLookup_(gate.context, request.path)
                : ApiResponse{503, "application/json",
                      "{\"error\":\"addon_activation_unavailable\"}", {}};
        }
        else if (pathWithoutQuery(request.path) ==
                "/api/v1/addons/rectools/sources")
        {
            apiResponse = addonSourceLookup_
                ? addonSourceLookup_(gate.context, request.path)
                : ApiResponse{503, "application/json",
                      "{\"error\":\"addon_sources_unavailable\"}", {}};
        }
        else if (pathWithoutQuery(request.path).rfind(
                "/api/v1/recording-playback-sessions", 0) == 0)
        {
            apiResponse = apiRouter_.handleDeviceRecordingPlayback(
                request.path, "", gate.context);
        }
        else if (pathWithoutQuery(request.path).rfind(
                "/api/v1/live-playback-sessions/", 0) == 0)
        {
            apiResponse = apiRouter_.handleDeviceLivePlayback(
                request.path, "", gate.context);
        }
        else
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
    }
    else if (request.method == "POST")
    {
        if (pathWithoutQuery(request.path) ==
                "/api/v1/addons/rectools/activation")
        {
            apiResponse = addonActivationMutation_
                ? addonActivationMutation_(gate.context, request.body)
                : ApiResponse{503, "application/json",
                      "{\"error\":\"addon_activation_unavailable\"}", {}};
        }
        else if (pathWithoutQuery(request.path) ==
                "/api/v1/addons/rectools/sources")
        {
            apiResponse = addonSourceMutation_
                ? addonSourceMutation_(gate.context, request.body)
                : ApiResponse{503, "application/json",
                      "{\"error\":\"addon_sources_unavailable\"}", {}};
        }
        else if (pathWithoutQuery(request.path).rfind(
                "/api/v1/recording-playback-sessions", 0) == 0)
        {
            apiResponse = apiRouter_.handleDeviceRecordingPlayback(
                request.path, request.body, gate.context);
        }
        else if (pathWithoutQuery(request.path).rfind(
                "/api/v1/live-playback-sessions", 0) == 0)
        {
            apiResponse = apiRouter_.handleDeviceLivePlayback(
                request.path, request.body, gate.context);
        }
        else
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
                gate.authorizationDecision.backendId,
                requestHeaderValue(
                    request,
                    "X-VDR-Suite-Pairing-Token"));
        }
    }
    else if (request.method == "DELETE")
    {
        apiResponse = apiRouter_.handleClientDelete(
            request.path,
            gate.context.actor.actorId,
            gate.context.requestId,
            gate.context.correlationId,
            requestHeaderValue(request, "If-Match"),
            requestHeaderValue(request, "Idempotency-Key"),
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

    // Fresh Device-grant/lifecycle/credential mutations revoke all active
    // subscriptions, forcing a new SecurityHttpGate admission.
    if (deviceEventHub_ && request.method == "POST" &&
        apiResponse.statusCode >= 200 && apiResponse.statusCode < 300 &&
        (pathWithoutQuery(request.path).rfind("/api/v1/devices/", 0) == 0 ||
         pathWithoutQuery(request.path) == "/api/v1/devices"))
        deviceEventHub_->disconnectAll();

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
