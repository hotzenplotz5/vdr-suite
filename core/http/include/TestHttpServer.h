#ifndef TEST_HTTP_SERVER_H
#define TEST_HTTP_SERVER_H

#include "ApiRouter.h"
#include "IEpgArtworkHttpProvider.h"
#include "IHttpServer.h"
#include "RestQueryParameters.h"

#include <string>

class ApiRouterHttpFacade
{
public:
    explicit ApiRouterHttpFacade(ApiRouter& apiRouter)
        : apiRouter_(apiRouter)
    {
    }

    ApiResponse handleGet(const std::string& requestTarget) const
    {
        const std::size_t queryStart = requestTarget.find('?');
        const std::string path = queryStart == std::string::npos
            ? requestTarget
            : requestTarget.substr(0, queryStart);

        if (path == "/api/person/context" ||
            path == "/api/persons/context")
        {
            const RestQueryParameters query = RestQueryParameters::parse(
                queryStart == std::string::npos
                    ? std::string{}
                    : requestTarget.substr(queryStart + 1));
            const std::string backend = query.get("backend").empty()
                ? query.get("backendId")
                : query.get("backend");
            const std::string providerPersonId =
                query.get("providerPersonId").empty()
                    ? query.get("tvscraperPersonId")
                    : query.get("providerPersonId");

            return apiRouter_.getPersonContext(
                query.get("name"),
                providerPersonId,
                backend,
                query.get("channelId"),
                query.get("eventId"),
                query.get("fromTime"),
                query.getInt("limit", 50),
                query.getInt("offset", 0));
        }

        return apiRouter_.handleGet(requestTarget);
    }

    ApiResponse handlePost(
        const std::string& requestTarget,
        const std::string& body) const
    {
        return apiRouter_.handlePost(requestTarget, body);
    }

    ApiResponse getEpgArtwork(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const
    {
        return apiRouter_.getEpgArtwork(
            backendId,
            channelId,
            eventId);
    }

private:
    ApiRouter& apiRouter_;
};

class TestHttpServer : public IHttpServer, public IEpgArtworkHttpProvider {
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

private:
    ApiRouterHttpFacade apiRouter_;

    HttpServerResponse mapApiResponse(
        int statusCode,
        const std::string& contentType,
        const std::string& body) const;
};

#endif
