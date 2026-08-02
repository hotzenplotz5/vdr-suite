#ifndef TEST_HTTP_SERVER_H
#define TEST_HTTP_SERVER_H

#include "AccountabilityEventRepository.h"
#include "ApiRouter.h"
#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionHttpGate.h"
#include "BrowserSessionHttpService.h"
#include "BrowserSessionIssuanceService.h"
#include "BrowserSessionLifecycleService.h"
#include "BrowserSessionRetentionService.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "IEpgArtworkHttpProvider.h"
#include "IHttpServer.h"
#include "ManagedBasicAuthenticator.h"
#include "PersistentIdentityResolver.h"
#include "SecurityHttpGate.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <memory>

class TestHttpServer : public IHttpServer, public IEpgArtworkHttpProvider
{
public:
    explicit TestHttpServer(ApiRouter& apiRouter);

    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override;

    HttpServerResponse getEpgArtwork(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const override
    {
        const ApiResponse response = apiRouter_.getEpgArtwork(
            backendId,
            channelId,
            eventId);

        return mapApiResponse(
            response.statusCode,
            response.contentType,
            response.body);
    }

    bool securityReady() const
    {
        return securityReady_;
    }

private:
    ApiRouter& apiRouter_;
    std::unique_ptr<Database> securityDatabase_;
    std::unique_ptr<AccountabilityEventRepository>
        accountabilityEventRepository_;
    std::unique_ptr<SecurityIdentityRepository>
        securityIdentityRepository_;
    std::unique_ptr<SecurityIdentityProvisioningRepository>
        securityIdentityProvisioningRepository_;
    std::unique_ptr<CredentialVerifierRepository>
        credentialVerifierRepository_;
    std::unique_ptr<BrowserSessionCredentialRepository>
        browserSessionCredentialRepository_;
    std::unique_ptr<SecurityPermissionGrantRepository>
        securityPermissionGrantRepository_;
    std::unique_ptr<BrowserSessionAuthenticator>
        browserSessionAuthenticator_;
    std::unique_ptr<ManagedBasicAuthenticator>
        managedBasicAuthenticator_;
    std::unique_ptr<PersistentIdentityResolver>
        persistentIdentityResolver_;
    std::unique_ptr<BrowserSessionIssuanceService>
        browserSessionIssuanceService_;
    std::unique_ptr<BrowserSessionLifecycleService>
        browserSessionLifecycleService_;
    std::unique_ptr<BrowserSessionRetentionService>
        browserSessionRetentionService_;
    std::unique_ptr<BrowserSessionHttpService>
        browserSessionHttpService_;
    std::unique_ptr<BrowserSessionHttpGate>
        browserSessionHttpGate_;
    std::unique_ptr<SecurityHttpGate> securityHttpGate_;
    bool securityReady_ = false;

    HttpServerResponse mapApiResponse(
        int statusCode,
        const std::string& contentType,
        const std::string& body) const;

    HttpServerResponse finalizeResponse(
        const RequestSecurityContext& context,
        HttpServerResponse response) const;
};

#endif