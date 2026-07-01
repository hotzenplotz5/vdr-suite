#include "TestHttpServer.h"

#include "ApiRouter.h"

#include <cstdlib>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string toLowerAscii(const std::string& value)
{
    std::string lowered;
    lowered.reserve(value.size());

    for (const char character : value)
    {
        lowered.push_back(
            static_cast<char>(
                std::tolower(
                    static_cast<unsigned char>(character))));
    }

    return lowered;
}

bool startsWith(
    const std::string& value,
    const std::string& prefix)
{
    return value.size() >= prefix.size() &&
        value.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(
    const std::string& value,
    const std::string& suffix)
{
    return value.size() >= suffix.size() &&
        value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int hexValue(const char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }

    if (character >= 'a' && character <= 'f')
    {
        return 10 + character - 'a';
    }

    if (character >= 'A' && character <= 'F')
    {
        return 10 + character - 'A';
    }

    return -1;
}

std::string percentDecodePath(const std::string& value)
{
    std::string decoded;
    decoded.reserve(value.size());

    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == '%' && index + 2 < value.size())
        {
            const int high = hexValue(value[index + 1]);
            const int low = hexValue(value[index + 2]);

            if (high >= 0 && low >= 0)
            {
                decoded.push_back(
                    static_cast<char>((high * 16) + low));
                index += 2;
                continue;
            }
        }

        decoded.push_back(value[index]);
    }

    return decoded;
}

bool isSafeRelativePath(const std::string& path)
{
    if (path.empty() ||
        path[0] == '/' ||
        path.find('\\') != std::string::npos ||
        path.find("//") != std::string::npos)
    {
        return false;
    }

    std::size_t segmentStart = 0;

    while (segmentStart <= path.size())
    {
        const std::size_t segmentEnd =
            path.find('/', segmentStart);
        const std::string segment =
            path.substr(
                segmentStart,
                segmentEnd == std::string::npos
                    ? std::string::npos
                    : segmentEnd - segmentStart);

        if (segment.empty() || segment == "." || segment == "..")
        {
            return false;
        }

        if (segmentEnd == std::string::npos)
        {
            break;
        }

        segmentStart = segmentEnd + 1;
    }

    return true;
}

bool isSupportedLogoPath(const std::string& path)
{
    const std::string lowered = toLowerAscii(path);

    return endsWith(lowered, ".png") ||
        endsWith(lowered, ".svg");
}

std::string logoContentType(const std::string& path)
{
    const std::string lowered = toLowerAscii(path);

    if (endsWith(lowered, ".svg"))
    {
        return "image/svg+xml";
    }

    return "image/png";
}

std::string headerValue(
    const HttpServerRequest& request,
    const std::string& headerName)
{
    const std::string wanted =
        toLowerAscii(headerName);

    for (const auto& header : request.headers)
    {
        if (toLowerAscii(header.first) == wanted)
        {
            return header.second;
        }
    }

    return "";
}

std::string expectedAuthorizationHeader()
{
    const char* configured =
        std::getenv("VDR_SUITE_BASIC_AUTH");

    if (configured != nullptr && configured[0] != '\0')
    {
        return configured;
    }

    return "Basic YWRtaW46dmRyLXN1aXRl";
}

bool isAuthorized(const HttpServerRequest& request)
{
    return headerValue(request, "Authorization") ==
        expectedAuthorizationHeader();
}

HttpServerResponse makeUnauthorizedResponse()
{
    HttpServerResponse response;

    response.statusCode = 401;
    response.headers["Content-Type"] = "application/json";
    response.headers["WWW-Authenticate"] =
        "Basic realm=\"VDR-Suite\", charset=\"UTF-8\"";
    response.body = "{\"error\":\"authentication required\"}";

    return response;
}

HttpServerResponse makeStaticNotFoundResponse()
{
    HttpServerResponse response;

    response.statusCode = 404;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"error\":\"frontend asset not found\"}";

    return response;
}

bool readFile(
    const std::string& path,
    std::string& content)
{
    std::ifstream file(path, std::ios::binary);

    if (!file)
    {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();

    return true;
}

std::vector<std::string> frontendRoots()
{
    const char* configuredRoot =
        std::getenv("VDR_SUITE_FRONTEND_ROOT");

    std::vector<std::string> roots;

    if (configuredRoot != nullptr && configuredRoot[0] != '\0')
    {
        roots.emplace_back(configuredRoot);
    }

    roots.emplace_back("web/frontend");
    roots.emplace_back("/usr/share/vdr-suite/web/frontend");
    roots.emplace_back("/usr/local/share/vdr-suite/web/frontend");

    return roots;
}

std::vector<std::string> channelLogoRoots()
{
    const char* configuredRoot =
        std::getenv("VDR_SUITE_CHANNEL_LOGO_ROOT");

    std::vector<std::string> roots;

    if (configuredRoot != nullptr && configuredRoot[0] != '\0')
    {
        roots.emplace_back(configuredRoot);
    }

    roots.emplace_back(
        "/usr/share/vdr/plugins/skindesigner/skins/estuary4vdr/logos");
    roots.emplace_back(
        "/usr/share/vdr/plugins/skindesigner/logos");

    return roots;
}

bool readFrontendAsset(
    const std::string& relativePath,
    std::string& content)
{
    for (const std::string& root : frontendRoots())
    {
        const std::string path =
            root + "/" + relativePath;

        if (readFile(path, content))
        {
            return true;
        }
    }

    return false;
}

bool readChannelLogoAsset(
    const std::string& relativePath,
    std::string& content)
{
    for (const std::string& root : channelLogoRoots())
    {
        const std::string path =
            root + "/" + relativePath;

        if (readFile(path, content))
        {
            return true;
        }
    }

    return false;
}

HttpServerResponse makeFrontendAssetResponse(
    const std::string& relativePath,
    const std::string& contentType)
{
    std::string content;

    if (!readFrontendAsset(relativePath, content))
    {
        return makeStaticNotFoundResponse();
    }

    HttpServerResponse response;

    response.statusCode = 200;
    response.headers["Content-Type"] = contentType;
    response.headers["Cache-Control"] = "no-store";
    response.body = content;

    return response;
}

HttpServerResponse makeChannelLogoResponse(
    const std::string& path)
{
    const std::string prefix = "/channel-logos/";
    const std::string relativePath =
        percentDecodePath(path.substr(prefix.size()));

    if (!isSafeRelativePath(relativePath) ||
        !isSupportedLogoPath(relativePath))
    {
        return makeStaticNotFoundResponse();
    }

    std::string content;

    if (!readChannelLogoAsset(relativePath, content))
    {
        return makeStaticNotFoundResponse();
    }

    HttpServerResponse response;

    response.statusCode = 200;
    response.headers["Content-Type"] = logoContentType(relativePath);
    response.headers["Cache-Control"] = "no-store";
    response.body = content;

    return response;
}

bool isFrontendPath(
    const std::string& path)
{
    return path == "/" ||
        path == "/frontend" ||
        path == "/frontend/" ||
        path == "/frontend/index.html" ||
        path == "/frontend/app.js" ||
        path == "/frontend/channel-logos.js" ||
        path == "/frontend/style.css";
}

bool isChannelLogoPath(
    const std::string& path)
{
    return startsWith(path, "/channel-logos/");
}

HttpServerResponse serveFrontendPath(
    const std::string& path)
{
    if (path == "/" ||
        path == "/frontend" ||
        path == "/frontend/" ||
        path == "/frontend/index.html")
    {
        return makeFrontendAssetResponse(
            "index.html",
            "text/html; charset=utf-8");
    }

    if (path == "/frontend/app.js")
    {
        return makeFrontendAssetResponse(
            "app.js",
            "application/javascript; charset=utf-8");
    }

    if (path == "/frontend/channel-logos.js")
    {
        return makeFrontendAssetResponse(
            "channel-logos.js",
            "application/javascript; charset=utf-8");
    }

    if (path == "/frontend/style.css")
    {
        return makeFrontendAssetResponse(
            "style.css",
            "text/css; charset=utf-8");
    }

    return makeStaticNotFoundResponse();
}

}

TestHttpServer::TestHttpServer(ApiRouter& apiRouter)
    : apiRouter_(apiRouter)
{
}

HttpServerResponse TestHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    if (!isAuthorized(request))
    {
        return makeUnauthorizedResponse();
    }

    if (request.method == "GET" &&
        isFrontendPath(request.path))
    {
        return serveFrontendPath(request.path);
    }

    if (request.method == "GET" &&
        isChannelLogoPath(request.path))
    {
        return makeChannelLogoResponse(request.path);
    }

    ApiResponse apiResponse;

    if (request.method == "GET")
    {
        apiResponse =
            apiRouter_.handleGet(request.path);
    }
    else if (request.method == "POST")
    {
        apiResponse =
            apiRouter_.handlePost(
                request.path,
                request.body);
    }
    else
    {
        return mapApiResponse(
            405,
            "application/json",
            "{\"error\":\"method not allowed\"}");
    }

    return mapApiResponse(
        apiResponse.statusCode,
        apiResponse.contentType,
        apiResponse.body);
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
