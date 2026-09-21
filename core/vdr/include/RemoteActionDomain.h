#pragma once

#include <string>
#include <vector>

enum class RemoteActionType
{
    Invalid,
    Up,
    Down,
    Left,
    Right,
    Ok,
    Back,
    Menu,
    Info,
    Red,
    Green,
    Yellow,
    Blue,
    Zero,
    One,
    Two,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    ChannelUp,
    ChannelDown,
    VolumeUp,
    VolumeDown,
    Mute,
    Play,
    Pause,
    Stop,
    Record,
    FastForward,
    Rewind,
    Next,
    Previous,
    SwitchChannel
};

enum class RemoteActionFailureKind
{
    None,
    Validation,
    BackendNotFound,
    Permission,
    Capability,
    ExecutorUnavailable,
    Transport,
    BackendRejected,
    BackendFailure
};

struct RemoteActionRequest
{
    std::string backendId;
    std::string operationId;
    RemoteActionType action = RemoteActionType::Invalid;
    std::string channelId;
};

struct RemoteActionResult
{
    bool success = false;
    RemoteActionFailureKind failureKind = RemoteActionFailureKind::None;
    std::string backendId;
    std::string operationId;
    RemoteActionType action = RemoteActionType::Invalid;
    std::string message;
    std::vector<std::string> errors;
    int backendStatusCode = 0;

    static RemoteActionResult ok(
        const RemoteActionRequest& request,
        const std::string& message);

    static RemoteActionResult failed(
        const RemoteActionRequest& request,
        RemoteActionFailureKind failureKind,
        const std::string& message,
        const std::vector<std::string>& errors = {},
        int backendStatusCode = 0);
};

RemoteActionType remoteActionTypeFromName(const std::string& name);
std::string remoteActionTypeName(RemoteActionType type);
std::string remoteActionFailureKindName(RemoteActionFailureKind kind);
bool isRemoteActionAllowlisted(RemoteActionType type);
bool isRemoteActionRequestToken(const std::string& value, std::size_t maximumLength);
