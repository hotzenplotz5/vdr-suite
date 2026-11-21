#include "RestfulApiVdrTimerActionExecutor.h"

#include "RestfulApiTimerMapper.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace
{
std::string buildUrl(
    const std::string& basePath,
    const std::string& endpoint)
{
    if (basePath.empty())
    {
        return endpoint;
    }

    if (basePath.back() == '/' && endpoint.front() == '/')
    {
        return basePath.substr(0, basePath.size() - 1) + endpoint;
    }

    if (basePath.back() != '/' && endpoint.front() != '/')
    {
        return basePath + "/" + endpoint;
    }

    return basePath + endpoint;
}

HttpRequest buildTimerReadbackRequest(
    const std::string& basePath)
{
    HttpRequest request;
    request.method = "GET";
    request.url = buildUrl(basePath, "/timers.json");
    request.headers["Accept"] = "application/json";
    return request;
}

std::string responseDiagnostic(
    const HttpResponse& response,
    const HttpRequest& request)
{
    std::string diagnostic =
        "RESTfulAPI returned HTTP status " +
        std::to_string(response.statusCode);

    diagnostic += " method=" + request.method;
    diagnostic += " url=" + request.url;

    if (!request.body.empty())
    {
        diagnostic += " requestBody=" + request.body;
    }

    if (!response.body.empty())
    {
        diagnostic += " responseBody=" + response.body;
    }

    return diagnostic;
}

int decimalValue(
    const std::string& value,
    int fallback)
{
    std::size_t position = 0;

    while (position < value.size() &&
           std::isspace(static_cast<unsigned char>(value.at(position))))
    {
        ++position;
    }

    if (position >= value.size())
    {
        return fallback;
    }

    int result = 0;
    bool parsed = false;

    while (position < value.size() &&
           std::isdigit(static_cast<unsigned char>(value.at(position))))
    {
        parsed = true;
        result = result * 10 + (value.at(position) - '0');
        ++position;
    }

    return parsed ? result : fallback;
}

bool timerMatchesRequest(
    const VdrTimer& timer,
    const VdrTimerOperationRequest& request)
{
    if (!request.channelId.empty() &&
        timer.channelId != request.channelId)
    {
        return false;
    }

    if (!request.day.empty() &&
        !timer.day.empty() &&
        timer.day != request.day)
    {
        return false;
    }

    if (decimalValue(timer.startTime, -1) != request.start ||
        decimalValue(timer.endTime, -1) != request.stop)
    {
        return false;
    }

    if (!request.title.empty() &&
        !timer.title.empty() &&
        timer.title != request.title)
    {
        return false;
    }

    if (!request.directory.empty() &&
        timer.directory != request.directory)
    {
        return false;
    }

    return !timer.id.empty();
}

const VdrTimer* matchingTimerFromReadback(
    const std::string& responseBody,
    const VdrTimerOperationRequest& request,
    std::vector<VdrTimer>& timers)
{
    timers = RestfulApiTimerMapper::parseTimers(responseBody);

    const auto match = std::find_if(
        timers.begin(),
        timers.end(),
        [&request](const VdrTimer& timer)
        {
            return timerMatchesRequest(timer, request);
        });

    return match == timers.end() ? nullptr : &(*match);
}
}

RestfulApiVdrTimerActionExecutor::RestfulApiVdrTimerActionExecutor(
    std::string backendId,
    std::string basePath,
    IHttpClient& httpClient)
    : backendId_(backendId),
      basePath_(basePath),
      httpClient_(httpClient)
{
}

VdrTimerActionResult RestfulApiVdrTimerActionExecutor::execute(
    VdrTimerActionType type,
    const VdrTimerOperationRequest& request)
{
    switch (type)
    {
        case VdrTimerActionType::Create:
            return executeBuiltRequest(
                type,
                request,
                requestBuilder_.buildCreateRequest(basePath_, request));

        case VdrTimerActionType::Update:
            return executeBuiltRequest(
                type,
                request,
                requestBuilder_.buildUpdateRequest(basePath_, request));

        case VdrTimerActionType::Delete:
            return executeBuiltRequest(
                type,
                request,
                requestBuilder_.buildDeleteRequest(basePath_, request));

        default:
            return VdrTimerActionResult::failed(
                type,
                request.timerId,
                backendId_,
                "RESTfulAPI timer action type not supported",
                {"unsupported timer action type for RESTfulAPI executor"});
    }
}

VdrTimerActionResult RestfulApiVdrTimerActionExecutor::executeBuiltRequest(
    VdrTimerActionType type,
    const VdrTimerOperationRequest& request,
    const HttpRequest& httpRequest) const
{
    HttpRequest preflightRequest;

    if (type == VdrTimerActionType::Create &&
        request.timerId.empty())
    {
        preflightRequest = buildTimerReadbackRequest(basePath_);

        const HttpResponse preflightResponse =
            httpClient_.execute(preflightRequest);

        if (preflightResponse.statusCode < 200 ||
            preflightResponse.statusCode >= 300)
        {
            return VdrTimerActionResult::failed(
                type,
                "",
                backendId_,
                "Timerprüfung vor dem Erstellen ist fehlgeschlagen.",
                {responseDiagnostic(preflightResponse, preflightRequest)});
        }

        std::vector<VdrTimer> existingTimers;
        const VdrTimer* existingTimer =
            matchingTimerFromReadback(
                preflightResponse.body,
                request,
                existingTimers);

        if (existingTimer != nullptr)
        {
            return VdrTimerActionResult::failed(
                type,
                existingTimer->id,
                backendId_,
                "Timer ist bereits vorhanden.",
                {"matching timer already existed before create request"});
        }
    }

    const HttpResponse response =
        httpClient_.execute(httpRequest);

    if (response.statusCode < 200 ||
        response.statusCode >= 300)
    {
        return VdrTimerActionResult::failed(
            type,
            request.timerId,
            backendId_,
            "RESTfulAPI timer action request failed",
            {responseDiagnostic(response, httpRequest)});
    }

    if (type == VdrTimerActionType::Create &&
        request.timerId.empty())
    {
        const HttpRequest readbackRequest =
            buildTimerReadbackRequest(basePath_);

        const HttpResponse readbackResponse =
            httpClient_.execute(readbackRequest);

        if (readbackResponse.statusCode < 200 ||
            readbackResponse.statusCode >= 300)
        {
            return VdrTimerActionResult::failed(
                type,
                "",
                backendId_,
                "RESTfulAPI timer creation readback failed",
                {responseDiagnostic(readbackResponse, readbackRequest)});
        }

        std::vector<VdrTimer> timers;
        const VdrTimer* matchingTimer =
            matchingTimerFromReadback(
                readbackResponse.body,
                request,
                timers);

        if (matchingTimer == nullptr)
        {
            return VdrTimerActionResult::failed(
                type,
                "",
                backendId_,
                "RESTfulAPI timer creation was not visible in readback",
                {"no timer matching channel=" + request.channelId +
                 " day=" + request.day +
                 " start=" + std::to_string(request.start) +
                 " stop=" + std::to_string(request.stop) +
                 " title=" + request.title +
                 " was present in GET " + readbackRequest.url});
        }

        return VdrTimerActionResult::ok(
            type,
            matchingTimer->id,
            backendId_,
            "RESTfulAPI timer creation confirmed by readback");
    }

    return VdrTimerActionResult::ok(
        type,
        request.timerId,
        backendId_,
        "RESTfulAPI timer action request executed");
}
