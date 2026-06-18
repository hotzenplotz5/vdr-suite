#pragma once

#include "HttpRequest.h"
#include "VdrTimerOperationRequest.h"

#include <sstream>
#include <string>

class RestfulApiVdrTimerActionRequestBuilder
{
public:
    HttpRequest buildCreateRequest(
        const std::string& basePath,
        const VdrTimerOperationRequest& request) const
    {
        HttpRequest httpRequest;
        httpRequest.method = "POST";
        httpRequest.url = buildUrl(basePath, "/timers");
        httpRequest.headers["Accept"] = "application/json";
        httpRequest.headers["Content-Type"] = "application/x-www-form-urlencoded";
        httpRequest.body = buildTimerBody(request, false);
        return httpRequest;
    }

    HttpRequest buildUpdateRequest(
        const std::string& basePath,
        const VdrTimerOperationRequest& request) const
    {
        HttpRequest httpRequest;
        httpRequest.method = "PUT";
        httpRequest.url = buildUrl(basePath, "/timers");
        httpRequest.headers["Accept"] = "application/json";
        httpRequest.headers["Content-Type"] = "application/x-www-form-urlencoded";
        httpRequest.body = buildTimerBody(request, true);
        return httpRequest;
    }

    HttpRequest buildDeleteRequest(
        const std::string& basePath,
        const VdrTimerOperationRequest& request) const
    {
        HttpRequest httpRequest;
        httpRequest.method = "DELETE";
        httpRequest.url = buildUrl(basePath, "/timers/" + request.timerId);
        httpRequest.headers["Accept"] = "application/json";
        return httpRequest;
    }

private:
    static std::string buildUrl(
        const std::string& basePath,
        const std::string& endpoint)
    {
        if (basePath.empty()) {
            return endpoint;
        }

        if (basePath.back() == '/' && endpoint.front() == '/') {
            return basePath.substr(0, basePath.size() - 1) + endpoint;
        }

        if (basePath.back() != '/' && endpoint.front() != '/') {
            return basePath + "/" + endpoint;
        }

        return basePath + endpoint;
    }

    static std::string boolFlag(bool enabled)
    {
        return enabled ? "1" : "0";
    }

    static std::string toString(int value)
    {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }

    static std::string fileName(const VdrTimerOperationRequest& request)
    {
        if (request.directory.empty()) {
            return request.title;
        }

        return request.directory + "~" + request.title;
    }

    static void appendParameter(
        std::string& body,
        const std::string& name,
        const std::string& value)
    {
        if (!body.empty()) {
            body += "&";
        }

        body += name;
        body += "=";
        body += value;
    }

    static std::string buildTimerBody(
        const VdrTimerOperationRequest& request,
        bool update)
    {
        std::string body;

        appendParameter(body, "flags", boolFlag(request.active));
        appendParameter(body, "channel", request.channelId);
        appendParameter(body, "weekdays", request.weekdays);
        appendParameter(body, "day", request.day);
        appendParameter(body, "start", toString(request.start));
        appendParameter(body, "stop", toString(request.stop));
        appendParameter(body, "priority", toString(request.priority));
        appendParameter(body, "lifetime", toString(request.lifetime));
        appendParameter(body, "file", fileName(request));
        appendParameter(body, "aux", request.aux);

        if (update) {
            appendParameter(body, "timer_id", request.timerId);
        }

        return body;
    }
};
