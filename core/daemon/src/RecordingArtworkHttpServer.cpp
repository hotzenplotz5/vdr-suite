#include "RecordingArtworkHttpServer.h"

#include "VdrRecordingCacheRepository.h"

#include <map>
#include <string>
#include <utility>

RecordingArtworkHttpServer::RecordingArtworkHttpServer(
    std::unique_ptr<IHttpServer> delegate,
    VdrRecordingCacheRepository& repository,
    std::map<std::string, std::string> artworkRootsByBackend)
    : delegate_(std::move(delegate)),
      artworkService_(
          repository,
          std::move(artworkRootsByBackend))
{
}

HttpServerResponse RecordingArtworkHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    const HttpServerResponse delegated =
        delegate_->handleRequest(request);

    if (request.method != "GET" ||
        !artworkService_.handlesPath(request.path) ||
        delegated.statusCode == 401 ||
        delegated.statusCode != 404)
    {
        return delegated;
    }

    const VdrRecordingArtworkAsset asset =
        artworkService_.loadPath(request.path);

    HttpServerResponse response;
    response.headers["X-Content-Type-Options"] = "nosniff";

    if (!asset.found())
    {
        response.statusCode = 404;
        response.headers["Content-Type"] = "application/json";
        response.headers["Cache-Control"] = "no-store";
        response.body =
            "{\"error\":\"recording artwork not found\"}";
        return response;
    }

    response.statusCode = 200;
    response.headers["Content-Type"] = asset.contentType;
    response.headers["Cache-Control"] =
        "private, max-age=300";
    response.body = asset.content;
    return response;
}
