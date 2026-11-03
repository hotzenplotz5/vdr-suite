#pragma once

#include "BackendAgentLifecycle.h"
#include "BackendAgentCommandDelivery.h"
#include "IHttpServer.h"

#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

class CredentialVerifierRepository;
class BackendAgentMediaImportCatalog;
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
        SecurityIdentityRepository& identityRepository,
        BackendAgentMediaImportCatalog* mediaImportCatalog = nullptr);

    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override;

    // Private authenticated Agent-only resolution. Callbacks must recheck the
    // live Core actor grant, command receipt, generation, operation and source.
    using MediaShrinkAuthorityLookup = std::function<std::string(
        const RequestSecurityContext&, const BackendAgentCommandReceipt&)>;
    void setMediaShrinkAuthorityLookup(MediaShrinkAuthorityLookup lookup);

private:
    RequestSecurityContext authenticateAgent(
        const HttpServerRequest& request) const;
    bool verifyAgentSecret(
        const std::string& agentId,
        const std::string& credentialId,
        const std::string& passwordHash,
        const std::string& secret) const;
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
    HttpServerResponse handleOsdObservation(const HttpServerRequest& request, const RequestSecurityContext& context) const;
    HttpServerResponse handleMediaImportSourceObservation(
        const HttpServerRequest& request,
        const RequestSecurityContext& context) const;
    HttpServerResponse handleCommandPoll(const HttpServerRequest& request, const RequestSecurityContext& context) const;
    HttpServerResponse handleCommandReceipt(const HttpServerRequest& request, const RequestSecurityContext& context) const;
    HttpServerResponse handleCommandResult(const HttpServerRequest& request, const RequestSecurityContext& context) const;
    HttpServerResponse handleMediaShrinkAuthority(
        const HttpServerRequest& request,
        const RequestSecurityContext& context) const;

    std::unique_ptr<IHttpServer> clientServer_;
    BackendAgentLifecycleService& lifecycleService_;
    BackendAgentCommandDeliveryService& commandDeliveryService_;
    BackendAgentRepository& repository_;
    CredentialVerifierRepository& credentialVerifierRepository_;
    SecurityIdentityRepository& identityRepository_;
    BackendAgentMediaImportCatalog* mediaImportCatalog_ = nullptr;
    MediaShrinkAuthorityLookup mediaShrinkAuthorityLookup_;

    // One successful Basic verifier, bound to the current durable credential
    // identity and hash. Revocation/expiry/permissions are read on every
    // request; a rotation changes the hash and misses the cache immediately.
    std::array<unsigned char, 32> secretCacheKey_{};
    bool secretCacheReady_ = false;
    mutable std::mutex secretCacheMutex_;
    mutable bool secretCachePresent_ = false;
    mutable std::string cachedAgentId_;
    mutable std::string cachedCredentialId_;
    mutable std::string cachedPasswordHash_;
    mutable std::array<unsigned char, 32> cachedSecretMac_{};
};
