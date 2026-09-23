#pragma once

#include <cstdint>
#include <string>

enum class PublicTimerCreateRequestParseStatus
{
    ok,
    invalidJson,
    invalidRequest,
    validationError,
};

struct PublicTimerCreateSpecification
{
    std::string title;
    std::string directory;
    std::string day;
    std::string weekdays;
    std::string startTime;
    std::string endTime;
    std::int32_t priority = 0;
    std::int32_t lifetime = 0;
    bool enabled = false;
    bool vps = false;
};

struct PublicTimerCreateRequestParseResult
{
    PublicTimerCreateRequestParseStatus status =
        PublicTimerCreateRequestParseStatus::invalidJson;
    PublicTimerCreateSpecification specification;
    std::string detail;

    bool ok() const
    {
        return status == PublicTimerCreateRequestParseStatus::ok;
    }
};

class PublicTimerCreateRequestParser
{
public:
    PublicTimerCreateRequestParseResult parse(
        const std::string& body) const;
};
