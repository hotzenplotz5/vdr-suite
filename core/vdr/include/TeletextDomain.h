#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class TeletextSourceState
{
    Unknown,
    Live,
    Cached
};

struct TeletextProviderEvidence
{
    std::string providerId;
    std::uint64_t capabilityRevision = 0;
    std::uint64_t providerGeneration = 0;
    std::uint64_t observedAt = 0;
};

struct TeletextServiceRef
{
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string channelId;
    TeletextProviderEvidence provider;
    bool available = false;
    bool receiverActive = false;
    TeletextSourceState source = TeletextSourceState::Unknown;
};

struct TeletextPageRef
{
    TeletextServiceRef service;
    std::uint16_t pageNumber = 0;
    std::uint16_t subpageCode = 0;
};

struct TeletextCell
{
    std::uint32_t codepoint = 0;
    std::uint8_t rawChar = 0;
    std::uint8_t charset = 0;
    std::uint8_t foreground = 0;
    std::uint8_t background = 0;
    std::uint8_t kind = 0;
    std::uint8_t flags = 0;
};

struct TeletextServiceSnapshot
{
    bool payloadValid = false;
    std::string error;
    TeletextServiceRef service;
    std::uint16_t rows = 0;
    std::uint16_t columns = 0;
    bool subpages = false;
    bool level1 = false;
    bool x26Partial = false;
    bool conceal = false;
    bool blink = false;
    bool doubleSize = false;
};

struct TeletextPageSnapshot
{
    bool payloadValid = false;
    bool pageAvailable = false;
    std::string result;
    std::string error;
    TeletextPageRef page;
    std::uint64_t revision = 0;
    bool complete = false;
    std::uint16_t rows = 0;
    std::uint16_t columns = 0;
    std::vector<std::string> textRows;
    std::vector<TeletextCell> cells;
};
