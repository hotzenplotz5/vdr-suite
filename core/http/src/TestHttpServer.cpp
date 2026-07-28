#include "TestHttpServer.h"

#include "ApiRouter.h"
#include "SecurityConfiguration.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

#include "TestHttpServerPaths.inc"
#include "TestHttpServerAssets.inc"
#include "TestHttpServerRoutes.inc"

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

    const SecurityConfiguration configuration =
        SecurityConfiguration::fromEnvironment();

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
        if (!configuration.managedBasic.complete() ||
            !ManagedBasicAuthenticator::supportsPasswordHash(
                configuration.managedBasic.passwordHash))
        {
            return;
        }

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

    securityHttpGate_ =
        std::make_unique<SecurityHttpGate>(
            configuration,
            *accountabilityEventRepository_,
            persistentIdentityResolver_.get(),
            managedBasicAuthenticator_.get());
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
    if (!securityReady_ || !securityHttpGate_)
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

    const SecurityGateDecision gate =
        securityHttpGate_->evaluate(request);

    if (!gate.allowed)
    {
        return gate.rejection;
    }

    if (request.method == "GET" &&
        isFrontendPath(request.path))
    {
        return finalizeResponse(
            gate.context,
            serveFrontendPath(request.path));
    }

    if (request.method == "GET" &&
        isChannelLogoPath(request.path))
    {
        return finalizeResponse(
            gate.context,
            makeChannelLogoResponse(request.path));
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
