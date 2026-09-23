#include "PublicApiContractRuntime.h"

#include "ServerBuildIdentity.h"

#include <string>

namespace
{

std::string requestPath(const std::string& requestTarget)
{
    const std::size_t separator = requestTarget.find('?');
    return separator == std::string::npos
        ? requestTarget
        : requestTarget.substr(0, separator);
}

ApiResponse jsonResponse(const std::string& body)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.body = body;
    return response;
}

ApiResponse contractRoot(const bool authenticated)
{
    return jsonResponse(
        std::string("{\"apiVersion\":\"v1\",\"serverVersion\":\"")
        + VdrSuiteServerBuildIdentity::ServerVersion
        + "\",\"supportedApiMajors\":[\"v1\"],\"authentication\":{\"authenticated\":"
        + (authenticated ? "true" : "false")
        + "},\"links\":{\"self\":\"/api/v1\",\"capabilities\":\"/api/v1/capabilities\"}}");
}

ApiResponse platformCapabilities()
{
    return jsonResponse(
        "{\"apiVersion\":\"v1\",\"capabilities\":["
        "{\"id\":\"public-api.contract-root\",\"version\":1,\"availability\":\"available\"}"
        "],\"links\":{\"self\":\"/api/v1/capabilities\",\"root\":\"/api/v1\"}}");
}

}

PublicApiContractRuntime& PublicApiContractRuntime::instance()
{
    static PublicApiContractRuntime runtime;
    return runtime;
}

bool PublicApiContractRuntime::tryHandleGet(
    const std::string& requestTarget,
    const std::string& actorRef,
    ApiResponse& response) const
{
    const std::string path = requestPath(requestTarget);

    if (path == "/api/v1")
    {
        response = contractRoot(!actorRef.empty());
        return true;
    }

    if (path == "/api/v1/capabilities")
    {
        response = platformCapabilities();
        return true;
    }

    return false;
}
