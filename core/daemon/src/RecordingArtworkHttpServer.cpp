#include "RecordingArtworkHttpServer.h"

#include "RestQueryParameters.h"
#include "VdrRecordingCacheRepository.h"

#include <map>
#include <string>
#include <utility>

namespace
{
std::string requestPath(const std::string& requestTarget)
{
    const std::size_t queryStart = requestTarget.find('?');
    return queryStart == std::string::npos
        ? requestTarget
        : requestTarget.substr(0, queryStart);
}

std::string requestQueryString(const std::string& requestTarget)
{
    const std::size_t queryStart = requestTarget.find('?');
    return queryStart == std::string::npos
        ? std::string{}
        : requestTarget.substr(queryStart + 1);
}

std::string normalizeBackendId(const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

bool metadataImagePath(const std::string& path)
{
    return path == "/api/vdr/recordings/metadata/image" ||
        path == "/api/recordings/metadata/image";
}

void applyPreview(const ArtworkPreviewCache& cache, const HttpServerRequest& request,
                  const RestQueryParameters& query, HttpServerResponse& response)
{
    if (response.statusCode != 200 || query.get("variant") != "home") return;
    const auto type = response.headers.find("Content-Type");
    if (type == response.headers.end()) return;
    const std::string preview = cache.home(request.path, type->second, response.body);
    if (preview.empty()) return;
    response.body = preview;
    response.headers["Content-Type"] = "image/jpeg";
    response.headers.erase("Content-Length");
    response.headers.erase("Content-Encoding");
    response.headers.erase("ETag");
}
}

RecordingArtworkHttpServer::RecordingArtworkHttpServer(
    std::unique_ptr<IHttpServer> delegate,
    VdrRecordingCacheRepository& repository,
    std::map<std::string, std::string> artworkRootsByBackend,
    std::string previewDirectory)
    : delegate_(std::move(delegate)),
      epgArtworkProvider_(
          dynamic_cast<IEpgArtworkHttpProvider*>(delegate_.get())),
      artworkService_(
          repository,
          std::move(artworkRootsByBackend)),
      previews_(std::move(previewDirectory))
{
}

HttpServerResponse RecordingArtworkHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    HttpServerResponse delegated =
        delegate_->handleRequest(request);
    const std::string path = requestPath(request.path);
    const RestQueryParameters query = RestQueryParameters::parse(requestQueryString(request.path));
    if (request.method == "GET" && delegated.statusCode != 401 && delegated.statusCode != 403 &&
        (metadataImagePath(path) || artworkService_.handlesPath(path)) &&
        query.has("variant") && query.get("variant") != "home")
    {
        HttpServerResponse invalid;
        invalid.statusCode = 400;
        invalid.headers["Content-Type"] = "application/json";
        invalid.headers["Cache-Control"] = "no-store";
        invalid.body = "{\"error\":\"unsupported artwork variant\"}";
        return invalid;
    }

    if (request.method == "GET" &&
        metadataImagePath(path) &&
        delegated.statusCode != 401)
    {
        const RestQueryParameters queryParameters =
            RestQueryParameters::parse(requestQueryString(request.path));
        const bool revisioned =
            !queryParameters.get("assignmentRevision").empty();
        delegated.headers["X-Content-Type-Options"] = "nosniff";
        delegated.headers["Cache-Control"] = delegated.statusCode == 200
            ? (revisioned
                ? "private, max-age=31536000, immutable"
                : "private, max-age=300")
            : "no-store";
        applyPreview(previews_, request, query, delegated);
        return delegated;
    }

    if (request.method != "GET" ||
        delegated.statusCode == 401 ||
        delegated.statusCode != 404)
    {
        return delegated;
    }

    if (path == "/api/epg/cache/artwork" &&
        epgArtworkProvider_ != nullptr)
    {
        const RestQueryParameters queryParameters =
            RestQueryParameters::parse(
                requestQueryString(request.path));

        HttpServerResponse response =
            epgArtworkProvider_->getEpgArtwork(
                normalizeBackendId(queryParameters.get("backend")),
                queryParameters.get("channelId"),
                queryParameters.get("eventId"));

        response.headers["X-Content-Type-Options"] = "nosniff";
        response.headers["Cache-Control"] =
            response.statusCode == 200
                ? "private, max-age=300"
                : "no-store";
        return response;
    }

    if (!artworkService_.handlesPath(request.path))
    {
        return delegated;
    }

    const VdrRecordingArtworkAsset asset =
        artworkService_.loadPath(path);

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
    applyPreview(previews_, request, query, response);
    return response;
}
