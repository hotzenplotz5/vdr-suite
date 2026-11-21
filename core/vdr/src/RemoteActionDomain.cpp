#include "RemoteActionDomain.h"

#include <cctype>
#include <map>

namespace
{
const std::map<std::string, RemoteActionType>& actionNames()
{
    static const std::map<std::string, RemoteActionType> names = {
        {"up", RemoteActionType::Up},
        {"down", RemoteActionType::Down},
        {"left", RemoteActionType::Left},
        {"right", RemoteActionType::Right},
        {"ok", RemoteActionType::Ok},
        {"back", RemoteActionType::Back},
        {"menu", RemoteActionType::Menu},
        {"info", RemoteActionType::Info},
        {"red", RemoteActionType::Red},
        {"green", RemoteActionType::Green},
        {"yellow", RemoteActionType::Yellow},
        {"blue", RemoteActionType::Blue},
        {"zero", RemoteActionType::Zero},
        {"one", RemoteActionType::One},
        {"two", RemoteActionType::Two},
        {"three", RemoteActionType::Three},
        {"four", RemoteActionType::Four},
        {"five", RemoteActionType::Five},
        {"six", RemoteActionType::Six},
        {"seven", RemoteActionType::Seven},
        {"eight", RemoteActionType::Eight},
        {"nine", RemoteActionType::Nine},
        {"channelUp", RemoteActionType::ChannelUp},
        {"channelDown", RemoteActionType::ChannelDown},
        {"volumeUp", RemoteActionType::VolumeUp},
        {"volumeDown", RemoteActionType::VolumeDown},
        {"mute", RemoteActionType::Mute},
        {"play", RemoteActionType::Play},
        {"pause", RemoteActionType::Pause},
        {"stop", RemoteActionType::Stop},
        {"record", RemoteActionType::Record},
        {"fastForward", RemoteActionType::FastForward},
        {"rewind", RemoteActionType::Rewind},
        {"next", RemoteActionType::Next},
        {"previous", RemoteActionType::Previous},
        {"switchChannel", RemoteActionType::SwitchChannel}
    };

    return names;
}
}

RemoteActionResult RemoteActionResult::ok(
    const RemoteActionRequest& request,
    const std::string& resultMessage)
{
    RemoteActionResult result;
    result.success = true;
    result.backendId = request.backendId;
    result.operationId = request.operationId;
    result.action = request.action;
    result.message = resultMessage;
    return result;
}

RemoteActionResult RemoteActionResult::failed(
    const RemoteActionRequest& request,
    RemoteActionFailureKind resultFailureKind,
    const std::string& resultMessage,
    const std::vector<std::string>& resultErrors,
    int resultBackendStatusCode)
{
    RemoteActionResult result;
    result.success = false;
    result.failureKind = resultFailureKind;
    result.backendId = request.backendId;
    result.operationId = request.operationId;
    result.action = request.action;
    result.message = resultMessage;
    result.errors = resultErrors;
    result.backendStatusCode = resultBackendStatusCode;
    return result;
}

RemoteActionType remoteActionTypeFromName(const std::string& name)
{
    const auto iterator = actionNames().find(name);
    return iterator == actionNames().end()
        ? RemoteActionType::Invalid
        : iterator->second;
}

std::string remoteActionTypeName(RemoteActionType type)
{
    for (const auto& entry : actionNames())
    {
        if (entry.second == type)
        {
            return entry.first;
        }
    }

    return "invalid";
}

std::string remoteActionFailureKindName(RemoteActionFailureKind kind)
{
    switch (kind)
    {
        case RemoteActionFailureKind::None: return "none";
        case RemoteActionFailureKind::Validation: return "validation";
        case RemoteActionFailureKind::BackendNotFound: return "backendNotFound";
        case RemoteActionFailureKind::Permission: return "permission";
        case RemoteActionFailureKind::Capability: return "capability";
        case RemoteActionFailureKind::ExecutorUnavailable: return "executorUnavailable";
        case RemoteActionFailureKind::Transport: return "transport";
        case RemoteActionFailureKind::BackendRejected: return "backendRejected";
        case RemoteActionFailureKind::BackendFailure: return "backendFailure";
    }

    return "backendFailure";
}

bool isRemoteActionAllowlisted(RemoteActionType type)
{
    return type != RemoteActionType::Invalid;
}

bool isRemoteActionRequestToken(
    const std::string& value,
    std::size_t maximumLength)
{
    if (value.empty() || value.size() > maximumLength)
    {
        return false;
    }

    for (const unsigned char character : value)
    {
        if (!std::isalnum(character) &&
            character != '-' &&
            character != '_' &&
            character != '.' &&
            character != ':')
        {
            return false;
        }
    }

    return true;
}
