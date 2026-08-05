#pragma once

#include "BackendAgentLifecycle.h"
#include "BackendAgentCommandDelivery.h"
#include "IHttpServer.h"

#include <memory>
#include <string>

class CredentialVerifierRepository;
class SecurityIdentityRepository;

class BackendAgentHttpServer : public IHttpServer
{
public:
    BackendAgentHttpServer(
        std::unique_ptr<IHttpServer> clientServer,
        BackendAgentLifecycleService& lifecycleService,
        BackendAgentCommandDeliveryService& commandDeliveryService,
        BackendAgentRepository& repository,
        CredentialVerifierRepository& credentialVerifierRepository,
        SecurityIdentityRepository& identityRepository);

    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override;

private:
    RequestSecurityContext authenticateAgent(
        const HttpServerRequest& request) const;
    HttpServerResponse handleEnrollment(
        const HttpServerRequest& request) const;
    HttpServerResponse handleCredentialRotation(
        const HttpServerRequest& request,
        const RequestSecurityContext& context) const;
    HttpServerResponse handleConnect(
        const HttpServerRequest& request,
        const RequestSecurityContext& context) const;
    HttpServerResponse handleHeartbeat(
        const HttpServerRequest& request,
        const RequestSecurityContext& context) const;
    HttpServerResponse handleCapabilities(
        const HttpServerRequest& request,
        const RequestSecurityContext& context) const;
    HttpServerResponse handleBackendHealthObservation(
        const HttpServerRequest& request,
        const RequestSecurityContext& context) const;
    HttpServerResponse handleChannelObservation(
        const HttpServerRequest& request,
        const RequestSecurityContext& context) const;
    HttpServerResponse handleCommandPoll(const HttpServerRequest& request, const RequestSecurityContext& context) const;
    HttpServerResponse handleCommandReceipt(const HttpServerRequest& request, const RequestSecurityContext& context) const;
    HttpServerResponse handleCommandResult(const HttpServerRequest& request, const RequestSecurityContext& context) const;

    std::unique_ptr<IHttpServer> clientServer_;
    BackendAgentLifecycleService& lifecycleService_;
    BackendAgentCommandDeliveryService& commandDeliveryService_;
    BackendAgentRepository& repository_;
    CredentialVerifierRepository& credentialVerifierRepository_;
    SecurityIdentityRepository& identityRepository_;
};
