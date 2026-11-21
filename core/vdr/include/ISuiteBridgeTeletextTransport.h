#pragma once

#include <cstdint>
#include <string>

enum class SuiteBridgeTeletextTransportStatus
{
    Success,
    Unavailable,
    Failed,
    Timeout
};

struct SuiteBridgeTeletextCommandReply
{
    bool transportSucceeded = false;
    SuiteBridgeTeletextTransportStatus transportStatus =
        SuiteBridgeTeletextTransportStatus::Failed;
    int replyCode = 0;
    std::string payload;
};

struct SuiteBridgeTeletextPageRequest
{
    std::string channelId;
    std::uint16_t pageNumber = 0;
    bool automaticSubpage = true;
    std::uint16_t subpageCode = 0;
};

class ISuiteBridgeTeletextTransport
{
public:
    virtual ~ISuiteBridgeTeletextTransport() = default;

    virtual SuiteBridgeTeletextCommandReply discoverTeletext() = 0;

    virtual SuiteBridgeTeletextCommandReply requestTeletextPage(
        const SuiteBridgeTeletextPageRequest& request) = 0;
};
