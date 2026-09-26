#pragma once

#include <cstdint>
#include <string>

enum class SuiteBridgeHbbtvTransportStatus
{
    Success,
    Unavailable,
    Failed,
    Timeout
};

struct SuiteBridgeHbbtvCommandReply
{
    bool transportSucceeded = false;
    SuiteBridgeHbbtvTransportStatus transportStatus =
        SuiteBridgeHbbtvTransportStatus::Failed;
    int replyCode = 0;
    std::string payload;
};

enum class SuiteBridgeHbbtvRuntimeOperation
{
    Launch,
    Status,
    Input,
    Close
};

enum class SuiteBridgeHbbtvInputAction
{
    None,
    Up,
    Down,
    Left,
    Right,
    Ok,
    Back,
    Red,
    Green,
    Yellow,
    Blue,
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
    Play,
    Pause,
    Stop,
    FastForward,
    Rewind
};

enum class SuiteBridgeHbbtvPresentationOperation
{
    Meta,
    Chunk
};

struct SuiteBridgeHbbtvPresentationRequest
{
    SuiteBridgeHbbtvPresentationOperation operation =
        SuiteBridgeHbbtvPresentationOperation::Meta;
    std::string sessionId;
    std::uint64_t frameRevision = 0;
    std::uint32_t offset = 0;
};

struct SuiteBridgeHbbtvMediaRequest
{
    std::string sessionId;
};

struct SuiteBridgeHbbtvRuntimeRequest
{
    SuiteBridgeHbbtvRuntimeOperation operation =
        SuiteBridgeHbbtvRuntimeOperation::Status;
    std::string sessionId;
    std::string channelId;
    std::uint32_t applicationId = 0;
    std::uint64_t descriptorRevision = 0;
    SuiteBridgeHbbtvInputAction inputAction =
        SuiteBridgeHbbtvInputAction::None;
};

class ISuiteBridgeHbbtvTransport
{
public:
    virtual ~ISuiteBridgeHbbtvTransport() = default;

    virtual SuiteBridgeHbbtvCommandReply discoverHbbtv(
        const std::string& channelId) = 0;

    virtual SuiteBridgeHbbtvCommandReply controlHbbtv(
        const SuiteBridgeHbbtvRuntimeRequest&)
    {
        return {};
    }

    virtual SuiteBridgeHbbtvCommandReply readHbbtvPresentation(
        const SuiteBridgeHbbtvPresentationRequest&)
    {
        return {};
    }

    virtual SuiteBridgeHbbtvCommandReply readHbbtvMedia(
        const SuiteBridgeHbbtvMediaRequest&)
    {
        return {};
    }
};
