#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

enum class LegacyOsdInputAction
{
    Up,
    Down,
    Left,
    Right,
    Ok,
    Back,
    Red,
    Green,
    Yellow,
    Blue
};

inline const char* legacyOsdInputActionName(LegacyOsdInputAction action)
{
    switch (action)
    {
        case LegacyOsdInputAction::Up: return "up";
        case LegacyOsdInputAction::Down: return "down";
        case LegacyOsdInputAction::Left: return "left";
        case LegacyOsdInputAction::Right: return "right";
        case LegacyOsdInputAction::Ok: return "ok";
        case LegacyOsdInputAction::Back: return "back";
        case LegacyOsdInputAction::Red: return "red";
        case LegacyOsdInputAction::Green: return "green";
        case LegacyOsdInputAction::Yellow: return "yellow";
        case LegacyOsdInputAction::Blue: return "blue";
    }
    return "";
}

inline bool legacyOsdInputActionFromName(
    const std::string& value,
    LegacyOsdInputAction& action)
{
    if (value == "up") action = LegacyOsdInputAction::Up;
    else if (value == "down") action = LegacyOsdInputAction::Down;
    else if (value == "left") action = LegacyOsdInputAction::Left;
    else if (value == "right") action = LegacyOsdInputAction::Right;
    else if (value == "ok") action = LegacyOsdInputAction::Ok;
    else if (value == "back") action = LegacyOsdInputAction::Back;
    else if (value == "red") action = LegacyOsdInputAction::Red;
    else if (value == "green") action = LegacyOsdInputAction::Green;
    else if (value == "yellow") action = LegacyOsdInputAction::Yellow;
    else if (value == "blue") action = LegacyOsdInputAction::Blue;
    else return false;
    return true;
}

struct LegacyOsdInputCommand
{
    static constexpr std::uint64_t SchemaVersion = 1;

    std::string inputCommandId;
    std::string legacyOsdSessionId;
    std::uint64_t sessionRevision = 0;
    std::string viewerBindingId;
    std::string controllerLeaseId;
    std::uint64_t controllerLeaseEpoch = 0;
    std::uint64_t leaseRevision = 0;
    std::string actorId;
    std::string clientInstanceId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string osdSurfaceId;
    std::string osdEpoch;
    LegacyOsdInputAction action = LegacyOsdInputAction::Up;
    std::string inputMode = "press";
    std::uint64_t repeatCount = 1;
    std::int64_t deadline = 0;
    std::string correlationId;
};

inline bool legacyOsdInputSafeToken(
    const std::string& value,
    bool allowColon = true,
    std::size_t maximumBytes = 128U)
{
    return !value.empty() && value.size() <= maximumBytes &&
        std::all_of(
            value.begin(), value.end(),
            [allowColon](unsigned char character) {
                return std::isalnum(character) != 0 ||
                    character == '-' || character == '_' ||
                    character == '.' || (allowColon && character == ':');
            });
}

inline bool legacyOsdInputCommandValid(const LegacyOsdInputCommand& command)
{
    return legacyOsdInputSafeToken(command.inputCommandId) &&
        legacyOsdInputSafeToken(command.legacyOsdSessionId, false) &&
        command.sessionRevision > 0 &&
        legacyOsdInputSafeToken(command.viewerBindingId, false) &&
        legacyOsdInputSafeToken(command.controllerLeaseId, false) &&
        command.controllerLeaseEpoch > 0 &&
        command.leaseRevision > 0 &&
        legacyOsdInputSafeToken(command.actorId) &&
        legacyOsdInputSafeToken(command.clientInstanceId) &&
        legacyOsdInputSafeToken(command.backendId, false) &&
        command.backendGeneration > 0 &&
        legacyOsdInputSafeToken(command.osdSurfaceId) &&
        legacyOsdInputSafeToken(command.osdEpoch, false) &&
        legacyOsdInputSafeToken(legacyOsdInputActionName(command.action), false) &&
        command.inputMode == "press" &&
        command.repeatCount == 1 &&
        command.deadline > 0 &&
        legacyOsdInputSafeToken(command.correlationId);
}

inline std::string legacyOsdInputCommandSerialize(
    const LegacyOsdInputCommand& command)
{
    if (!legacyOsdInputCommandValid(command)) return {};
    return std::string("osdi1|") +
        command.inputCommandId + "|" +
        command.legacyOsdSessionId + "|" +
        std::to_string(command.sessionRevision) + "|" +
        command.viewerBindingId + "|" +
        command.controllerLeaseId + "|" +
        std::to_string(command.controllerLeaseEpoch) + "|" +
        std::to_string(command.leaseRevision) + "|" +
        command.actorId + "|" +
        command.clientInstanceId + "|" +
        command.backendId + "|" +
        std::to_string(command.backendGeneration) + "|" +
        command.osdSurfaceId + "|" +
        command.osdEpoch + "|" +
        legacyOsdInputActionName(command.action) + "|" +
        command.inputMode + "|" +
        std::to_string(command.repeatCount) + "|" +
        std::to_string(command.deadline);
}

inline bool legacyOsdInputParseUnsigned(
    const std::string& value,
    std::uint64_t& output)
{
    if (value.empty()) return false;
    std::uint64_t parsed = 0;
    for (unsigned char character : value)
    {
        if (character < '0' || character > '9') return false;
        const std::uint64_t digit =
            static_cast<std::uint64_t>(character - '0');
        if (parsed >
            (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
            return false;
        parsed = parsed * 10U + digit;
    }
    output = parsed;
    return true;
}

inline bool legacyOsdInputCommandParse(
    const std::string& encoded,
    LegacyOsdInputCommand& command)
{
    if (encoded.empty() || encoded.size() > 2048U) return false;
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (start <= encoded.size())
    {
        const std::size_t separator = encoded.find('|', start);
        fields.push_back(encoded.substr(
            start,
            separator == std::string::npos
                ? std::string::npos
                : separator - start));
        if (separator == std::string::npos) break;
        start = separator + 1U;
    }
    if (fields.size() != 18U || fields[0] != "osdi1") return false;

    LegacyOsdInputCommand parsed;
    std::uint64_t deadline = 0;
    if (!legacyOsdInputParseUnsigned(fields[3], parsed.sessionRevision) ||
        !legacyOsdInputParseUnsigned(fields[6], parsed.controllerLeaseEpoch) ||
        !legacyOsdInputParseUnsigned(fields[7], parsed.leaseRevision) ||
        !legacyOsdInputParseUnsigned(fields[11], parsed.backendGeneration) ||
        !legacyOsdInputActionFromName(fields[14], parsed.action) ||
        !legacyOsdInputParseUnsigned(fields[16], parsed.repeatCount) ||
        !legacyOsdInputParseUnsigned(fields[17], deadline) ||
        deadline > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
        return false;

    parsed.inputCommandId = fields[1];
    parsed.legacyOsdSessionId = fields[2];
    parsed.viewerBindingId = fields[4];
    parsed.controllerLeaseId = fields[5];
    parsed.actorId = fields[8];
    parsed.clientInstanceId = fields[9];
    parsed.backendId = fields[10];
    parsed.osdSurfaceId = fields[12];
    parsed.osdEpoch = fields[13];
    parsed.inputMode = fields[15];
    parsed.deadline = static_cast<std::int64_t>(deadline);
    parsed.correlationId = "internal";
    if (!legacyOsdInputCommandValid(parsed)) return false;
    command = parsed;
    return true;
}
