#ifndef TEST_HTTP_SERVER_H
#define TEST_HTTP_SERVER_H

#include "AccountabilityEventRepository.h"
#include "ApiRouter.h"
#include "Database.h"
#include "IEpgArtworkHttpProvider.h"
#include "IHttpServer.h"
#include "PersistentIdentityResolver.h"
#include "SecurityHttpGate.h"
#include "SecurityIdentityRepository.h"

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
    std::unique_ptr<PersistentIdentityResolver>
        persistentIdentityResolver_;
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
