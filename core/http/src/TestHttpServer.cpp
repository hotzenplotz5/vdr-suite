#include "TestHttpServer.h"

#include "ApiRouter.h"
#include "BrowserSessionCsrfRecoveryService.h"
#include "SecurityConfiguration.h"

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

    if (!configuration.expectedAuthorizationHeader.empty() &&
        !securityIdentityRepository_->ensureCompatibilityIdentity(
            configuration.actorId,
            ActorType::User,
            configuration.actorDisplayName,
            configuration.deviceId,
            configuration.sessionId,
            configuration.credentialId))
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
            BrowserSessionIdleConfiguration::LastSeenWriteIntervalSeconds);

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
            managedBasicAuthenticator_.get());

    securityHttpGate_ =
        std::make_unique<SecurityHttpGate>(
            configuration,
            *accountabilityEventRepository_,
            persistentIdentityResolver_.get(),
            managedBasicAuthenticator_.get(),
            browserSessionAuthenticator_.get());
    securityReady_ = true;
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
        !browserSessionHttpService_)
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

    const SecurityGateDecision gate =
        securityHttpGate_->evaluate(request);

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
            apiRouter_.handleClientGet(request.path);
    }
    else if (request.method == "POST")
    {
        apiResponse =
            apiRouter_.handleClientPost(
                request.path,
                request.body);
    }
    else
    {
        return finalizeResponse(
            gate.context,
            mapApiResponse(
                405,
                "application/json",
                "{\"error\":\"method not allowed\"}"));
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
            apiResponse.body));
}

HttpServerResponse TestHttpServer::mapApiResponse(
    int statusCode,
    const std::string& contentType,
    const std::string& body) const
{
    HttpServerResponse response;
    response.statusCode = statusCode;
    response.headers["Content-Type"] = contentType;
    response.body = body;
    return response;
}