#include "RecordingArtworkHttpServer.h"

#include "RestQueryParameters.h"
#include "ArtworkCacheValidation.h"
#include "VdrRecordingCacheRepository.h"

#include <chrono>
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

bool seriesImagePath(const std::string& path)
{
    const std::string prefix = "/api/backends/";
    const std::string suffix = "/settings/series-artwork/image";
    return path.size() > prefix.size() + suffix.size() &&
        path.compare(0, prefix.size(), prefix) == 0 &&
        path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool epgImagePath(const std::string& path)
{
    return path == "/api/epg/cache/artwork" ||
        path == "/api/epg/cache/metadata/image";
}

void applyPreview(const ArtworkPreviewCache& cache, const HttpServerRequest& request,
                  const RestQueryParameters& query, HttpServerResponse& response,
                  std::chrono::steady_clock::time_point requestStarted)
{
    if (response.statusCode != 200 || query.get("variant") != "home") return;
    // An unscoped preview (including legacy compatibility paths) is still
    // served normally, but must not create an unvalidated HTTP cache hit.
    if (response.headers.find("X-VDR-Suite-Artwork-Scope") == response.headers.end())
        response.headers["Cache-Control"] = "no-store";
    const auto type = response.headers.find("Content-Type");
    if (type == response.headers.end()) return;
    const auto previewStarted = std::chrono::steady_clock::now();
    const std::string preview = cache.home(request.path, type->second, response.body);
    const auto previewFinished = std::chrono::steady_clock::now();
    using Milliseconds = std::chrono::duration<double, std::milli>;
    // Same-origin timing evidence; read includes authorization, source lookup
    // and original byte delivery, and preview includes cache/decoder work.
    const std::string timing = "artwork-read;dur=" +
        std::to_string(Milliseconds(previewStarted - requestStarted).count()) +
        ", artwork-preview;dur=" +
        std::to_string(Milliseconds(previewFinished - previewStarted).count());
    auto& serverTiming = response.headers["Server-Timing"];
    if (!serverTiming.empty()) serverTiming += ", ";
    serverTiming += timing;
    if (!preview.empty()) {
        response.body = preview;
        response.headers["Content-Type"] = "image/jpeg";
        response.headers.erase("Content-Length");
        response.headers.erase("Content-Encoding");
        response.headers.erase("ETag");
        response.headers.erase("Last-Modified");
    }
    vdrsuite::http::revalidateScopedArtwork(request, response);
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
      artworkService_(std::make_shared<VdrRecordingArtworkService>(
          repository, std::move(artworkRootsByBackend))),
      previews_(std::move(previewDirectory))
{
}

RecordingArtworkHttpServer::RecordingArtworkHttpServer(
    std::unique_ptr<IHttpServer> delegate,
    std::shared_ptr<VdrRecordingArtworkService> artworkService,
    std::string previewDirectory)
    : delegate_(std::move(delegate)),
      epgArtworkProvider_(
          dynamic_cast<IEpgArtworkHttpProvider*>(delegate_.get())),
      artworkService_(std::move(artworkService)),
      previews_(std::move(previewDirectory))
{
}

HttpServerResponse RecordingArtworkHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    const auto requestStarted = std::chrono::steady_clock::now();
    HttpServerResponse delegated =
        delegate_->handleRequest(request);
    const std::string path = requestPath(request.path);
    const RestQueryParameters query = RestQueryParameters::parse(requestQueryString(request.path));
    if (request.method == "GET" && delegated.statusCode != 401 && delegated.statusCode != 403 &&
        (metadataImagePath(path) || epgImagePath(path) || artworkService_->handlesPath(path)) &&
        query.has("variant") && query.get("variant") != "home")
    {
        HttpServerResponse invalid;
        invalid.statusCode = 400;
        invalid.headers["Content-Type"] = "application/json";
        invalid.headers["Cache-Control"] = "no-store";
        invalid.body = "{\"error\":\"unsupported artwork variant\"}";
        return invalid;
    }

    // The selected series-cover route is already owned by the settings API.
    // Resize only explicit Home requests, preserving detail/candidate images
    // and original statuses; use the existing final-byte scoped validator.
    if (request.method == "GET" && seriesImagePath(path) &&
        delegated.statusCode == 200 && query.get("variant") == "home")
    {
        applyPreview(previews_, request, query, delegated, requestStarted);
        return delegated;
    }

    // Preserve the original Hero/detail bytes; resize only Home variants.
    // The existing compatibility fallback still owns its 404 path.
    if (request.method == "GET" && epgImagePath(path) &&
        delegated.statusCode == 200)
    {
        if (query.get("variant") == "home")
            applyPreview(previews_, request, query, delegated, requestStarted);
        else
            vdrsuite::http::revalidateScopedArtwork(request, delegated);
        return delegated;
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
        applyPreview(previews_, request, query, delegated, requestStarted);
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
        const auto scope = delegated.headers.find("X-VDR-Suite-Artwork-Scope");
        if (response.statusCode == 200 && scope != delegated.headers.end())
            response.headers["X-VDR-Suite-Artwork-Scope"] = scope->second;
        if (query.get("variant") == "home")
            applyPreview(previews_, request, query, response, requestStarted);
        else
            vdrsuite::http::revalidateScopedArtwork(request, response);
        return response;
    }

    if (!artworkService_->handlesPath(request.path))
    {
        return delegated;
    }

    const VdrRecordingArtworkAsset asset =
        artworkService_->loadPath(path);

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
    // Only the router may attach the already-authorized cache scope.
    // The fallback cannot synthesize scope from a URL or a missing asset.
    const auto scope = delegated.headers.find("X-VDR-Suite-Artwork-Scope");
    if (query.get("variant") == "home" && scope != delegated.headers.end())
        response.headers["X-VDR-Suite-Artwork-Scope"] = scope->second;
    applyPreview(previews_, request, query, response, requestStarted);
    return response;
}
