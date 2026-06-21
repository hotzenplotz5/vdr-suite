#include "RestfulApiSearchTimerCommandExecutor.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpClient.h"
#include "SearchTimer.h"
#include "SearchTimerCreateRequest.h"
#include "SearchTimerCreateResult.h"
#include "SearchTimerDeleteRequest.h"
#include "SearchTimerDeleteResult.h"
#include "SearchTimerUpdateRequest.h"
#include "SearchTimerUpdateResult.h"

#include <cctype>
#include <sstream>
#include <string>

namespace
{
std::string urlEncode(
    const std::string& value)
{
    std::ostringstream encoded;

    for (const unsigned char character : value)
    {
        if (std::isalnum(character) ||
            character == '-' ||
            character == '_' ||
            character == '.' ||
            character == '~')
        {
            encoded << character;
        }
        else
        {
            const char* digits = "0123456789ABCDEF";
            encoded << '%';
            encoded << digits[(character >> 4) & 0x0F];
            encoded << digits[character & 0x0F];
        }
    }

    return encoded.str();
}

std::string buildCreateUrl(
    const SearchTimerCreateRequest& request)
{
    std::string url = "/searchtimers";

    url += "?search=";
    url += urlEncode(request.query);

    url += "&use_title=1";
    url += "&use_subtitle=1";
    url += "&use_description=1";
    url += "&use_channel=0";
    url += "&use_as_searchtimer=";
    url += request.active ? "1" : "0";

    return url;
}

std::string extractCreatedId(
    const std::string& body)
{
    const std::string marker = "Id:";
    const std::size_t markerPosition =
        body.find(marker);

    if (markerPosition == std::string::npos)
    {
        return "";
    }

    std::size_t idStart =
        markerPosition + marker.size();

    while (idStart < body.size() &&
           std::isspace(static_cast<unsigned char>(body[idStart])))
    {
        ++idStart;
    }

    std::size_t idEnd = idStart;

    while (idEnd < body.size() &&
           std::isdigit(static_cast<unsigned char>(body[idEnd])))
    {
        ++idEnd;
    }

    return body.substr(idStart, idEnd - idStart);
}
}

RestfulApiSearchTimerCommandExecutor::RestfulApiSearchTimerCommandExecutor(
    IHttpClient& httpClient)
    : httpClient_(httpClient)
{
}

SearchTimerCreateResult RestfulApiSearchTimerCommandExecutor::create(
    const SearchTimerCreateRequest& request)
{
    HttpRequest httpRequest;
    httpRequest.method = "POST";
    httpRequest.url = buildCreateUrl(request);
    httpRequest.headers["Accept"] = "text/plain";

    const HttpResponse response =
        httpClient_.execute(httpRequest);

    if (response.statusCode != 200)
    {
        return SearchTimerCreateResult::failed(
            "RESTfulAPI searchtimer create failed",
            {response.body});
    }

    const std::string createdId =
        extractCreatedId(response.body);

    if (createdId.empty())
    {
        return SearchTimerCreateResult::failed(
            "RESTfulAPI searchtimer create did not return an id",
            {response.body});
    }

    return SearchTimerCreateResult::ok(
        SearchTimer::create(
            SearchTimerId::fromBackendNativeId(
                request.backendId,
                createdId),
            request.name,
            request.query,
            request.active
                ? SearchTimerState::Active
                : SearchTimerState::Inactive),
        "searchtimer created through RESTfulAPI");
}

SearchTimerUpdateResult RestfulApiSearchTimerCommandExecutor::update(
    const SearchTimerUpdateRequest& request)
{
    (void)request;

    return SearchTimerUpdateResult::failed(
        "RESTfulAPI searchtimer update is not supported",
        {"RESTfulAPI POST /searchtimers/update only triggers epgsearch update and does not edit an existing searchtimer"});
}

SearchTimerDeleteResult RestfulApiSearchTimerCommandExecutor::remove(
    const SearchTimerDeleteRequest& request)
{
    HttpRequest httpRequest;
    httpRequest.method = "DELETE";
    httpRequest.url = "/searchtimers/";
    httpRequest.url += urlEncode(request.backendNativeId);
    httpRequest.headers["Accept"] = "text/plain";

    const HttpResponse response =
        httpClient_.execute(httpRequest);

    if (response.statusCode != 200)
    {
        return SearchTimerDeleteResult::failed(
            "RESTfulAPI searchtimer delete failed",
            {response.body});
    }

    return SearchTimerDeleteResult::ok(
        request.backendId,
        request.backendNativeId,
        "searchtimer deleted through RESTfulAPI");
}
