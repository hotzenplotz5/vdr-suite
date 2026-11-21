#include "RestfulApiRemoteActionExecutor.h"

#include "HttpRequest.h"
#include "HttpResponse.h"

#include <cctype>
#include <exception>
#include <map>
#include <sstream>

RestfulApiRemoteActionExecutor::RestfulApiRemoteActionExecutor(
    std::string backendId,
    std::string basePath,
    IHttpClient& httpClient)
    : backendId_(std::move(backendId)),
      basePath_(std::move(basePath)),
      httpClient_(httpClient)
{
}

std::string RestfulApiRemoteActionExecutor::endpointForAction(
    RemoteActionType action)
{
    static const std::map<RemoteActionType, std::string> endpoints = {
        {RemoteActionType::Up, "up"},
        {RemoteActionType::Down, "down"},
        {RemoteActionType::Left, "left"},
        {RemoteActionType::Right, "right"},
        {RemoteActionType::Ok, "ok"},
        {RemoteActionType::Back, "back"},
        {RemoteActionType::Menu, "menu"},
        {RemoteActionType::Info, "info"},
        {RemoteActionType::Red, "red"},
        {RemoteActionType::Green, "green"},
        {RemoteActionType::Yellow, "yellow"},
        {RemoteActionType::Blue, "blue"},
        {RemoteActionType::Zero, "0"},
        {RemoteActionType::One, "1"},
        {RemoteActionType::Two, "2"},
        {RemoteActionType::Three, "3"},
        {RemoteActionType::Four, "4"},
        {RemoteActionType::Five, "5"},
        {RemoteActionType::Six, "6"},
        {RemoteActionType::Seven, "7"},
        {RemoteActionType::Eight, "8"},
        {RemoteActionType::Nine, "9"},
        {RemoteActionType::ChannelUp, "chanup"},
        {RemoteActionType::ChannelDown, "chandn"},
        {RemoteActionType::VolumeUp, "volup"},
        {RemoteActionType::VolumeDown, "voldn"},
        {RemoteActionType::Mute, "mute"},
        {RemoteActionType::Play, "play"},
        {RemoteActionType::Pause, "pause"},
        {RemoteActionType::Stop, "stop"},
        {RemoteActionType::Record, "record"},
        {RemoteActionType::FastForward, "fastfwd"},
        {RemoteActionType::Rewind, "fastrew"},
        {RemoteActionType::Next, "next"},
        {RemoteActionType::Previous, "prev"}
    };

    const auto iterator = endpoints.find(action);
    return iterator == endpoints.end() ? "" : iterator->second;
}

std::string RestfulApiRemoteActionExecutor::percentEncodePathSegment(
    const std::string& value)
{
    std::ostringstream encoded;
    encoded << std::uppercase << std::hex;

    for (const unsigned char character : value)
    {
        if (std::isalnum(character) ||
            character == '-' ||
            character == '_' ||
            character == '.' ||
            character == '~')
        {
            encoded << static_cast<char>(character);
        }
        else
        {
            encoded << '%';
            encoded.width(2);
            encoded.fill('0');
            encoded << static_cast<int>(character);
        }
    }

    return encoded.str();
}

std::string RestfulApiRemoteActionExecutor::buildUrl(
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

RemoteActionResult RestfulApiRemoteActionExecutor::execute(
    const RemoteActionRequest& request) const
{
    if (request.backendId != backendId_)
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::Validation,
            "Remote action executor backend mismatch",
            {"executor backend is " + backendId_});
    }

    std::string endpoint;

    if (request.action == RemoteActionType::SwitchChannel)
    {
        endpoint = "/remote/switch/" + percentEncodePathSegment(request.channelId);
    }
    else
    {
        const std::string backendKey = endpointForAction(request.action);

        if (backendKey.empty())
        {
            return RemoteActionResult::failed(
                request,
                RemoteActionFailureKind::Validation,
                "Remote action is not supported by the RESTfulAPI adapter",
                {"unknown action was not sent"});
        }

        endpoint = "/remote/" + backendKey;
    }

    HttpRequest httpRequest;
    httpRequest.method = "POST";
    httpRequest.url = buildUrl(basePath_, endpoint);
    httpRequest.headers["Accept"] = "application/json, text/plain;q=0.8";

    HttpResponse response;

    try
    {
        response = httpClient_.execute(httpRequest);
    }
    catch (const std::exception& error)
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::Transport,
            "RESTfulAPI remote transport failed",
            {error.what()});
    }
    catch (...)
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::Transport,
            "RESTfulAPI remote transport failed",
            {"unknown transport exception"});
    }

    if (response.statusCode >= 200 && response.statusCode < 300)
    {
        return RemoteActionResult::ok(
            request,
            request.action == RemoteActionType::SwitchChannel
                ? "Channel switch executed"
                : "Remote action executed");
    }

    if (response.statusCode <= 0)
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::Transport,
            "RESTfulAPI remote transport failed",
            {"transport returned no HTTP status"});
    }

    const std::string diagnostic =
        "RESTfulAPI returned HTTP " +
        std::to_string(response.statusCode) +
        (response.body.empty() ? "" : ": " + response.body);

    if (response.statusCode >= 400 && response.statusCode < 500)
    {
        return RemoteActionResult::failed(
            request,
            RemoteActionFailureKind::BackendRejected,
            "RESTfulAPI rejected the remote action",
            {diagnostic},
            response.statusCode);
    }

    return RemoteActionResult::failed(
        request,
        RemoteActionFailureKind::BackendFailure,
        "RESTfulAPI remote action failed",
        {diagnostic},
        response.statusCode);
}
