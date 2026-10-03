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
#include "FirstAdminBootstrapRepository.h"
#include "FirstAdminClaimHttpService.h"
#include "FirstAdminClaimService.h"
#include "HumanAccountAdministrationRepository.h"
#include "HumanAccountAdministrationService.h"
#include "HumanAccountCreationRepository.h"
#include "HumanAccountCreationService.h"
#include "HumanAccountCredentialSessionReadRepository.h"
#include "HumanAccountCredentialSessionReadService.h"
#include "HumanAccountGrantAdministrationService.h"
#include "HumanAccountSessionAdministrationService.h"
#include "HumanAccountRepository.h"
#include "HumanPasswordBrowserAuthenticator.h"
#include "Database.h"
#include "IEpgArtworkHttpProvider.h"
#include "IHttpServer.h"
#include "ManagedBasicAuthenticator.h"
#include "PersistentIdentityResolver.h"
#include "SecurityHttpGate.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <map>
#include <memory>

class TestHttpServer : public IHttpServer, public IEpgArtworkHttpProvider
{
public:
    explicit TestHttpServer(ApiRouter& apiRouter);
    ~TestHttpServer() override;

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
            response.body,
            response.headers);
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
    std::unique_ptr<HumanAccountRepository>
        humanAccountRepository_;
    std::unique_ptr<HumanAccountAdministrationRepository>
        humanAccountAdministrationRepository_;
    std::unique_ptr<HumanAccountAdministrationService>
        humanAccountAdministrationService_;
    std::unique_ptr<HumanAccountCreationRepository>
        humanAccountCreationRepository_;
    std::unique_ptr<HumanAccountCreationService>
        humanAccountCreationService_;
    std::unique_ptr<HumanAccountGrantAdministrationService>
        humanAccountGrantAdministrationService_;
    std::unique_ptr<HumanAccountCredentialSessionReadRepository>
        humanAccountCredentialSessionReadRepository_;
    std::unique_ptr<HumanAccountCredentialSessionReadService>
        humanAccountCredentialSessionReadService_;
    std::unique_ptr<HumanAccountSessionAdministrationService>
        humanAccountSessionAdministrationService_;
    std::unique_ptr<FirstAdminBootstrapRepository>
        firstAdminBootstrapRepository_;
    std::unique_ptr<FirstAdminClaimService>
        firstAdminClaimService_;
    std::unique_ptr<FirstAdminClaimHttpService>
        firstAdminClaimHttpService_;
    std::unique_ptr<BrowserSessionCredentialRepository>
        browserSessionCredentialRepository_;
    std::unique_ptr<SecurityPermissionGrantRepository>
        securityPermissionGrantRepository_;
    std::unique_ptr<BrowserSessionAuthenticator>
        browserSessionAuthenticator_;
    std::unique_ptr<ManagedBasicAuthenticator>
        managedBasicAuthenticator_;
    std::unique_ptr<HumanPasswordBrowserAuthenticator>
        humanPasswordBrowserAuthenticator_;
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
        const std::string& body,
        const std::map<std::string, std::string>& headers) const;

    HttpServerResponse finalizeResponse(
        const RequestSecurityContext& context,
        HttpServerResponse response) const;
};

#endif