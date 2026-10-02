#include "PublicApiRuntime.h"

#include "PublicProblemDetails.h"
#include "PublicResourcePreconditions.h"
#include "ServerBuildIdentity.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{

std::string requestPath(const std::string& requestTarget)
{
    const std::size_t separator = requestTarget.find('?');
    return separator == std::string::npos
        ? requestTarget
        : requestTarget.substr(0, separator);
}

std::string requestQueryString(const std::string& requestTarget)
{
    const std::size_t separator = requestTarget.find('?');
    return separator == std::string::npos
        ? std::string()
        : requestTarget.substr(separator + 1U);
}

constexpr const char* PublicApiV1Root = "/api/v1";
constexpr const char* PublicOperationPrefix = "/api/v1/operations/";
constexpr const char* PublicTimerAssignmentCollectionPath =
    "/api/v1/timer-assignments";
constexpr const char* PublicTimerAssignmentPrefix =
    "/api/v1/timer-assignments/";
constexpr std::size_t PublicTimerAssignmentDefaultLimit = 50U;
constexpr std::size_t PublicTimerAssignmentMaximumLimit = 100U;
constexpr const char* PublicTimerAssignmentCollectionSort =
    "timerAssignmentId";
constexpr const char* PublicTimerAssignmentCollectionOrder = "asc";
constexpr const char* PublicTimerAssignmentCursorPrefix = "ta1_";
constexpr const char* PublicTimerAssignmentCursorPayloadVersion =
    "timer-assignments/1|";
constexpr const char* PublicBackendCollectionPath =
    "/api/v1/backends";
constexpr std::size_t PublicBackendDefaultLimit = 50U;
constexpr std::size_t PublicBackendMaximumLimit = 100U;
constexpr const char* PublicBackendCollectionSort = "backendId";
constexpr const char* PublicBackendCollectionOrder = "asc";
constexpr const char* PublicBackendCursorPrefix = "be1_";
constexpr const char* PublicBackendCursorPayloadVersion =
    "backends/1|";
constexpr const char* PublicAccountCollectionPath =
    "/api/v1/accounts";
constexpr const char* PublicAccountPrefix =
    "/api/v1/accounts/";
constexpr std::size_t PublicAccountDefaultLimit = 50U;
constexpr std::size_t PublicAccountMaximumLimit = 100U;
constexpr const char* PublicAccountCollectionSort = "accountId";
constexpr const char* PublicAccountCollectionOrder = "asc";
constexpr const char* PublicAccountCursorPrefix = "ac1_";
constexpr const char* PublicAccountCursorPayloadVersion =
    "accounts/1|";
constexpr const char* PublicChannelCollectionPath =
    "/api/v1/channels";
constexpr std::size_t PublicChannelDefaultLimit = 50U;
constexpr std::size_t PublicChannelMaximumLimit = 100U;
constexpr std::size_t PublicChannelMaximumSources = 16U;
constexpr const char* PublicChannelCollectionSort =
    "backendId,channelId";
constexpr const char* PublicChannelCollectionOrder = "asc";
constexpr const char* PublicChannelCursorPrefix = "ch1_";
constexpr const char* PublicChannelCursorPayloadVersion =
    "channels/1|";

bool isPublicV1Path(const std::string& path)
{
    const std::string root(PublicApiV1Root);
    return path == root ||
        (path.size() > root.size() &&
         path.compare(0, root.size(), root) == 0 &&
         path[root.size()] == '/');
}

bool publicOperationPath(
    const std::string& path,
    std::string& operationId)
{
    const std::string prefix(PublicOperationPrefix);

    if (path.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    operationId = path.substr(prefix.size());
    return !operationId.empty() &&
        operationId.find('/') == std::string::npos;
}

bool publicAccountPath(
    const std::string& path,
    std::string& accountId)
{
    const std::string prefix(PublicAccountPrefix);

    if (path.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    accountId = path.substr(prefix.size());
    return !accountId.empty() &&
        accountId.find('/') == std::string::npos;
}

bool publicAccountGrantPath(
    const std::string& path,
    std::string& accountId)
{
    const std::string prefix(PublicAccountPrefix);
    static const std::string Suffix = "/grants";

    if (path.compare(0, prefix.size(), prefix) != 0 ||
        path.size() <= prefix.size() + Suffix.size() ||
        path.compare(
            path.size() - Suffix.size(),
            Suffix.size(),
            Suffix) != 0)
    {
        return false;
    }

    accountId = path.substr(
        prefix.size(),
        path.size() - prefix.size() - Suffix.size());
    return !accountId.empty() &&
        accountId.find('/') == std::string::npos;
}

bool publicTimerAssignmentPath(
    const std::string& path,
    std::string& timerAssignmentId)
{
    const std::string prefix(PublicTimerAssignmentPrefix);

    if (path.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    timerAssignmentId = path.substr(prefix.size());
    return !timerAssignmentId.empty() &&
        timerAssignmentId.find('/') == std::string::npos;
}

struct PublicTimerAssignmentCollectionQuery
{
    std::size_t limit = PublicTimerAssignmentDefaultLimit;
    std::string cursor;
};

bool decimalSize(const std::string& value, std::size_t& parsed)
{
    if (value.empty()) return false;
    std::size_t result = 0U;
    for (const unsigned char character : value)
    {
        if (character < '0' || character > '9') return false;
        const std::size_t digit =
            static_cast<std::size_t>(character - '0');
        if (result > 1000000U) return false;
        result = result * 10U + digit;
    }
    parsed = result;
    return true;
}

bool parsePublicTimerAssignmentCollectionQuery(
    const std::string& requestTarget,
    PublicTimerAssignmentCollectionQuery& query)
{
    const std::string encoded = requestQueryString(requestTarget);
    if (encoded.empty()) return false;

    bool backendSeen = false;
    bool limitSeen = false;
    bool cursorSeen = false;
    bool sortSeen = false;
    bool orderSeen = false;
    std::size_t position = 0U;

    while (position <= encoded.size())
    {
        const std::size_t separator = encoded.find('&', position);
        const std::string item = encoded.substr(
            position,
            separator == std::string::npos
                ? std::string::npos
                : separator - position);
        if (item.empty()) return false;

        const std::size_t equals = item.find('=');
        if (equals == std::string::npos) return false;
        const std::string key = item.substr(0U, equals);
        const std::string value = item.substr(equals + 1U);

        if (key == "backend")
        {
            if (backendSeen || value.empty()) return false;
            backendSeen = true;
        }
        else if (key == "limit")
        {
            if (limitSeen) return false;
            limitSeen = true;
            std::size_t parsed = 0U;
            if (!decimalSize(value, parsed) ||
                parsed == 0U ||
                parsed > PublicTimerAssignmentMaximumLimit)
                return false;
            query.limit = parsed;
        }
        else if (key == "cursor")
        {
            if (cursorSeen || value.empty() || value.size() > 2048U)
                return false;
            cursorSeen = true;
            query.cursor = value;
        }
        else if (key == "sort")
        {
            if (sortSeen ||
                value != PublicTimerAssignmentCollectionSort)
                return false;
            sortSeen = true;
        }
        else if (key == "order")
        {
            if (orderSeen ||
                value != PublicTimerAssignmentCollectionOrder)
                return false;
            orderSeen = true;
        }
        else
        {
            return false;
        }

        if (separator == std::string::npos) break;
        position = separator + 1U;
    }
    return backendSeen;
}

struct PublicBackendCollectionQuery
{
    std::size_t limit = PublicBackendDefaultLimit;
    std::string cursor;
};

bool parsePublicBackendCollectionQuery(
    const std::string& requestTarget,
    PublicBackendCollectionQuery& query)
{
    const std::string encoded = requestQueryString(requestTarget);
    if (encoded.empty()) return true;

    bool limitSeen = false;
    bool cursorSeen = false;
    bool sortSeen = false;
    bool orderSeen = false;
    std::size_t position = 0U;

    while (position <= encoded.size())
    {
        const std::size_t separator = encoded.find('&', position);
        const std::string item = encoded.substr(
            position,
            separator == std::string::npos
                ? std::string::npos
                : separator - position);
        if (item.empty()) return false;

        const std::size_t equals = item.find('=');
        if (equals == std::string::npos) return false;
        const std::string key = item.substr(0U, equals);
        const std::string value = item.substr(equals + 1U);

        if (key == "limit")
        {
            if (limitSeen) return false;
            limitSeen = true;
            std::size_t parsed = 0U;
            if (!decimalSize(value, parsed) ||
                parsed == 0U ||
                parsed > PublicBackendMaximumLimit)
                return false;
            query.limit = parsed;
        }
        else if (key == "cursor")
        {
            if (cursorSeen || value.empty() || value.size() > 4096U)
                return false;
            cursorSeen = true;
            query.cursor = value;
        }
        else if (key == "sort")
        {
            if (sortSeen || value != PublicBackendCollectionSort)
                return false;
            sortSeen = true;
        }
        else if (key == "order")
        {
            if (orderSeen || value != PublicBackendCollectionOrder)
                return false;
            orderSeen = true;
        }
        else
        {
            return false;
        }

        if (separator == std::string::npos) break;
        position = separator + 1U;
    }

    return true;
}

struct PublicAccountCollectionQuery
{
    std::size_t limit = PublicAccountDefaultLimit;
    std::string cursor;
};

bool parsePublicAccountCollectionQuery(
    const std::string& requestTarget,
    PublicAccountCollectionQuery& query)
{
    const std::string encoded = requestQueryString(requestTarget);
    if (encoded.empty()) return true;

    bool limitSeen = false;
    bool cursorSeen = false;
    bool sortSeen = false;
    bool orderSeen = false;
    std::size_t position = 0U;

    while (position <= encoded.size())
    {
        const std::size_t separator = encoded.find('&', position);
        const std::string item = encoded.substr(
            position,
            separator == std::string::npos
                ? std::string::npos
                : separator - position);
        if (item.empty()) return false;

        const std::size_t equals = item.find('=');
        if (equals == std::string::npos) return false;
        const std::string key = item.substr(0U, equals);
        const std::string value = item.substr(equals + 1U);

        if (key == "limit")
        {
            if (limitSeen) return false;
            limitSeen = true;
            std::size_t parsed = 0U;
            if (!decimalSize(value, parsed) ||
                parsed == 0U ||
                parsed > PublicAccountMaximumLimit)
                return false;
            query.limit = parsed;
        }
        else if (key == "cursor")
        {
            if (cursorSeen || value.empty() || value.size() > 4096U)
                return false;
            cursorSeen = true;
            query.cursor = value;
        }
        else if (key == "sort")
        {
            if (sortSeen || value != PublicAccountCollectionSort)
                return false;
            sortSeen = true;
        }
        else if (key == "order")
        {
            if (orderSeen || value != PublicAccountCollectionOrder)
                return false;
            orderSeen = true;
        }
        else
        {
            return false;
        }

        if (separator == std::string::npos) break;
        position = separator + 1U;
    }

    return true;
}

struct PublicChannelCollectionQuery
{
    std::size_t limit = PublicChannelDefaultLimit;
    std::size_t backendCount = 0U;
    std::string cursor;
};

bool parsePublicChannelCollectionQuery(
    const std::string& requestTarget,
    PublicChannelCollectionQuery& query)
{
    const std::string encoded = requestQueryString(requestTarget);
    if (encoded.empty()) return false;

    bool limitSeen = false;
    bool cursorSeen = false;
    bool sortSeen = false;
    bool orderSeen = false;
    std::size_t position = 0U;

    while (position <= encoded.size())
    {
        const std::size_t separator = encoded.find('&', position);
        const std::string item = encoded.substr(
            position,
            separator == std::string::npos
                ? std::string::npos
                : separator - position);
        if (item.empty()) return false;

        const std::size_t equals = item.find('=');
        if (equals == std::string::npos) return false;
        const std::string key = item.substr(0U, equals);
        const std::string value = item.substr(equals + 1U);

        if (key == "backendId")
        {
            if (value.empty() ||
                ++query.backendCount > PublicChannelMaximumSources)
                return false;
        }
        else if (key == "limit")
        {
            if (limitSeen) return false;
            limitSeen = true;
            std::size_t parsed = 0U;
            if (!decimalSize(value, parsed) ||
                parsed == 0U ||
                parsed > PublicChannelMaximumLimit)
                return false;
            query.limit = parsed;
        }
        else if (key == "cursor")
        {
            if (cursorSeen || value.empty() || value.size() > 4096U)
                return false;
            cursorSeen = true;
            query.cursor = value;
        }
        else if (key == "sort")
        {
            if (sortSeen || value != PublicChannelCollectionSort)
                return false;
            sortSeen = true;
        }
        else if (key == "order")
        {
            if (orderSeen || value != PublicChannelCollectionOrder)
                return false;
            orderSeen = true;
        }
        else
        {
            return false;
        }

        if (separator == std::string::npos) break;
        position = separator + 1U;
    }

    return query.backendCount > 0U;
}

char hexDigit(unsigned int value)
{
    return static_cast<char>(
        value < 10U ? ('0' + value) : ('a' + (value - 10U)));
}

std::string hexEncode(const std::string& value)
{
    std::string encoded;
    encoded.reserve(value.size() * 2U);
    for (const unsigned char character : value)
    {
        encoded.push_back(hexDigit(character >> 4U));
        encoded.push_back(hexDigit(character & 0x0fU));
    }
    return encoded;
}

int hexValue(char value)
{
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

bool hexDecode(const std::string& encoded, std::string& decoded)
{
    if (encoded.empty() ||
        encoded.size() % 2U != 0U ||
        encoded.size() > 4096U)
        return false;

    decoded.clear();
    decoded.reserve(encoded.size() / 2U);
    for (std::size_t index = 0U;
         index < encoded.size();
         index += 2U)
    {
        const int high = hexValue(encoded[index]);
        const int low = hexValue(encoded[index + 1U]);
        if (high < 0 || low < 0) return false;
        decoded.push_back(static_cast<char>((high << 4) | low));
    }
    return true;
}

void appendCursorField(std::string& payload, const std::string& value)
{
    payload += std::to_string(value.size());
    payload.push_back(':');
    payload += value;
}

bool readCursorField(
    const std::string& payload,
    std::size_t& position,
    std::string& value)
{
    if (position >= payload.size()) return false;
    std::size_t length = 0U;
    bool digitSeen = false;
    while (position < payload.size() && payload[position] != ':')
    {
        const unsigned char character = payload[position++];
        if (character < '0' || character > '9') return false;
        digitSeen = true;
        if (length > 4096U) return false;
        length = length * 10U +
            static_cast<std::size_t>(character - '0');
    }
    if (!digitSeen ||
        position >= payload.size() ||
        payload[position] != ':')
        return false;
    ++position;
    if (length > payload.size() - position) return false;
    value = payload.substr(position, length);
    position += length;
    return true;
}

std::string publicTimerAssignmentCursor(
    const std::string& backendId,
    const std::string& lastTimerAssignmentId)
{
    if (backendId.empty() ||
        lastTimerAssignmentId.empty())
        return "";

    std::string payload(PublicTimerAssignmentCursorPayloadVersion);
    appendCursorField(payload, backendId);
    appendCursorField(payload, lastTimerAssignmentId);
    return std::string(PublicTimerAssignmentCursorPrefix) +
        hexEncode(payload);
}

bool decodePublicTimerAssignmentCursor(
    const std::string& cursor,
    const std::string& backendId,
    std::string& lastTimerAssignmentId)
{
    const std::string prefix(PublicTimerAssignmentCursorPrefix);
    if (cursor.size() <= prefix.size() ||
        cursor.compare(0U, prefix.size(), prefix) != 0)
        return false;

    std::string payload;
    if (!hexDecode(cursor.substr(prefix.size()), payload))
        return false;

    const std::string version(
        PublicTimerAssignmentCursorPayloadVersion);
    if (payload.compare(0U, version.size(), version) != 0)
        return false;

    std::size_t position = version.size();
    std::string cursorBackend;
    std::string cursorLast;
    if (!readCursorField(payload, position, cursorBackend) ||
        !readCursorField(payload, position, cursorLast) ||
        position != payload.size())
        return false;

    if (cursorBackend != backendId ||
        cursorLast.empty() ||
        cursorLast.size() > 160U)
        return false;

    lastTimerAssignmentId = cursorLast;
    return true;
}

std::string normalizedPublicBackendAuthorizationScope(
    std::vector<std::string> backendIds)
{
    std::sort(backendIds.begin(), backendIds.end());
    std::string scope = std::to_string(backendIds.size()) + "|";
    for (const std::string& backendId : backendIds)
        appendCursorField(scope, backendId);
    return scope;
}

enum class PublicBackendCursorDecodeStatus
{
    ok,
    invalid,
    scopeMismatch,
};

std::string publicBackendCursor(
    const std::string& authorizationScope,
    const std::string& lastBackendId)
{
    if (authorizationScope.empty() || lastBackendId.empty())
        return "";

    std::string payload(PublicBackendCursorPayloadVersion);
    appendCursorField(payload, authorizationScope);
    appendCursorField(payload, lastBackendId);
    return std::string(PublicBackendCursorPrefix) + hexEncode(payload);
}

PublicBackendCursorDecodeStatus decodePublicBackendCursor(
    const std::string& cursor,
    const std::string& authorizationScope,
    std::string& lastBackendId)
{
    const std::string prefix(PublicBackendCursorPrefix);
    if (cursor.size() <= prefix.size() ||
        cursor.compare(0U, prefix.size(), prefix) != 0)
        return PublicBackendCursorDecodeStatus::invalid;

    std::string payload;
    if (!hexDecode(cursor.substr(prefix.size()), payload))
        return PublicBackendCursorDecodeStatus::invalid;

    const std::string version(PublicBackendCursorPayloadVersion);
    if (payload.compare(0U, version.size(), version) != 0)
        return PublicBackendCursorDecodeStatus::invalid;

    std::size_t position = version.size();
    std::string cursorScope;
    if (!readCursorField(payload, position, cursorScope) ||
        !readCursorField(payload, position, lastBackendId) ||
        position != payload.size() ||
        lastBackendId.empty() ||
        lastBackendId.size() > 128U)
        return PublicBackendCursorDecodeStatus::invalid;

    if (cursorScope != authorizationScope)
        return PublicBackendCursorDecodeStatus::scopeMismatch;

    return PublicBackendCursorDecodeStatus::ok;
}

std::string publicAccountCursor(
    const std::string& lastAccountId)
{
    if (lastAccountId.empty()) return "";

    std::string payload(PublicAccountCursorPayloadVersion);
    appendCursorField(payload, lastAccountId);
    return std::string(PublicAccountCursorPrefix) + hexEncode(payload);
}

bool decodePublicAccountCursor(
    const std::string& cursor,
    std::string& lastAccountId)
{
    const std::string prefix(PublicAccountCursorPrefix);
    if (cursor.size() <= prefix.size() ||
        cursor.compare(0U, prefix.size(), prefix) != 0)
        return false;

    std::string payload;
    if (!hexDecode(cursor.substr(prefix.size()), payload))
        return false;

    const std::string version(PublicAccountCursorPayloadVersion);
    if (payload.compare(0U, version.size(), version) != 0)
        return false;

    std::size_t position = version.size();
    if (!readCursorField(payload, position, lastAccountId) ||
        position != payload.size() ||
        lastAccountId.empty() ||
        lastAccountId.size() > 160U)
        return false;

    return true;
}

std::string normalizedPublicChannelBackendScope(
    std::vector<std::string> backendIds)
{
    std::sort(backendIds.begin(), backendIds.end());
    std::string scope;
    for (std::size_t index = 0U; index < backendIds.size(); ++index)
    {
        if (index > 0U) scope.push_back(',');
        scope += backendIds[index];
    }
    return scope;
}

enum class PublicChannelCursorDecodeStatus
{
    ok,
    invalid,
    scopeMismatch,
};

std::string publicChannelCursor(
    const std::string& backendScope,
    const std::string& lastBackendId,
    const std::string& lastChannelId)
{
    if (backendScope.empty() ||
        lastBackendId.empty() ||
        lastChannelId.empty())
        return "";

    std::string payload(PublicChannelCursorPayloadVersion);
    appendCursorField(payload, backendScope);
    appendCursorField(payload, lastBackendId);
    appendCursorField(payload, lastChannelId);
    return std::string(PublicChannelCursorPrefix) + hexEncode(payload);
}

PublicChannelCursorDecodeStatus decodePublicChannelCursor(
    const std::string& cursor,
    const std::string& backendScope,
    std::string& lastBackendId,
    std::string& lastChannelId)
{
    const std::string prefix(PublicChannelCursorPrefix);
    if (cursor.size() <= prefix.size() ||
        cursor.compare(0U, prefix.size(), prefix) != 0)
        return PublicChannelCursorDecodeStatus::invalid;

    std::string payload;
    if (!hexDecode(cursor.substr(prefix.size()), payload))
        return PublicChannelCursorDecodeStatus::invalid;

    const std::string version(PublicChannelCursorPayloadVersion);
    if (payload.compare(0U, version.size(), version) != 0)
        return PublicChannelCursorDecodeStatus::invalid;

    std::size_t position = version.size();
    std::string cursorScope;
    if (!readCursorField(payload, position, cursorScope) ||
        !readCursorField(payload, position, lastBackendId) ||
        !readCursorField(payload, position, lastChannelId) ||
        position != payload.size() ||
        lastBackendId.empty() ||
        lastChannelId.empty() ||
        lastBackendId.size() > 128U ||
        lastChannelId.size() > 256U)
        return PublicChannelCursorDecodeStatus::invalid;

    if (cursorScope != backendScope)
        return PublicChannelCursorDecodeStatus::scopeMismatch;

    return PublicChannelCursorDecodeStatus::ok;
}

std::string publicBackendCollectionTarget(
    std::size_t limit,
    const std::string& cursor)
{
    std::string target =
        std::string(PublicBackendCollectionPath) +
        "?limit=" + std::to_string(limit) +
        "&sort=" + PublicBackendCollectionSort +
        "&order=" + PublicBackendCollectionOrder;
    if (!cursor.empty())
        target += "&cursor=" + cursor;
    return target;
}

std::string publicAccountCollectionTarget(
    std::size_t limit,
    const std::string& cursor)
{
    std::string target =
        std::string(PublicAccountCollectionPath) +
        "?limit=" + std::to_string(limit) +
        "&sort=" + PublicAccountCollectionSort +
        "&order=" + PublicAccountCollectionOrder;
    if (!cursor.empty())
        target += "&cursor=" + cursor;
    return target;
}

std::string publicChannelCollectionTarget(
    std::vector<std::string> backendIds,
    std::size_t limit,
    const std::string& cursor)
{
    std::sort(backendIds.begin(), backendIds.end());
    std::string target(PublicChannelCollectionPath);
    for (std::size_t index = 0U; index < backendIds.size(); ++index)
    {
        target += index == 0U ? "?backendId=" : "&backendId=";
        target += backendIds[index];
    }
    target += "&limit=" + std::to_string(limit) +
        "&sort=" + PublicChannelCollectionSort +
        "&order=" + PublicChannelCollectionOrder;
    if (!cursor.empty())
        target += "&cursor=" + cursor;
    return target;
}

std::string publicTimerAssignmentCollectionTarget(
    const std::string& backendId,
    std::size_t limit,
    const std::string& cursor)
{
    std::string target =
        std::string(PublicTimerAssignmentCollectionPath) +
        "?backend=" + backendId +
        "&limit=" + std::to_string(limit) +
        "&sort=" + PublicTimerAssignmentCollectionSort +
        "&order=" + PublicTimerAssignmentCollectionOrder;
    if (!cursor.empty())
        target += "&cursor=" + cursor;
    return target;
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());

    for (const unsigned char character : value)
    {
        switch (character)
        {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (character >= 0x20U)
                {
                    escaped.push_back(
                        static_cast<char>(character));
                }
                break;
        }
    }

    return escaped;
}

std::string lowerAscii(std::string value)
{
    for (char& character : value)
    {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

bool asciiWhitespace(char character)
{
    return character == ' ' || character == '\t' ||
        character == '\r' || character == '\n';
}

std::string trimAsciiWhitespace(std::string value)
{
    std::size_t begin = 0;
    while (begin < value.size() && asciiWhitespace(value[begin]))
        ++begin;

    std::size_t end = value.size();
    while (end > begin && asciiWhitespace(value[end - 1U]))
        --end;

    return value.substr(begin, end - begin);
}

bool applicationJsonContentType(const std::string& value)
{
    if (value.empty()) return false;
    const std::size_t separator = value.find(';');
    const std::string mediaType = trimAsciiWhitespace(
        value.substr(0, separator));
    return lowerAscii(mediaType) == "application/json";
}

bool publicIdempotencyKeyValid(const std::string& value)
{
    if (value.empty() || value.size() > 160U) return false;
    for (const unsigned char character : value)
    {
        if (character < 0x21U || character > 0x7eU)
            return false;
    }
    return true;
}

class JsonSyntaxValidator
{
public:
    explicit JsonSyntaxValidator(const std::string& input)
        : input_(input)
    {
    }

    bool valid()
    {
        position_ = 0;
        skipWhitespace();
        if (!parseValue(0U)) return false;
        skipWhitespace();
        return position_ == input_.size();
    }

private:
    void skipWhitespace()
    {
        while (position_ < input_.size() &&
               asciiWhitespace(input_[position_]))
        {
            ++position_;
        }
    }

    bool parseValue(std::size_t depth)
    {
        if (depth > 16U || position_ >= input_.size())
            return false;

        const char current = input_[position_];
        if (current == '"') return parseString();
        if (current == '{') return parseObject(depth + 1U);
        if (current == '[') return parseArray(depth + 1U);
        if (current == 't') return parseLiteral("true");
        if (current == 'f') return parseLiteral("false");
        if (current == 'n') return parseLiteral("null");
        return parseNumber();
    }

    bool parseLiteral(const char* literal)
    {
        const std::string value(literal);
        if (input_.compare(position_, value.size(), value) != 0)
            return false;
        position_ += value.size();
        return true;
    }

    bool parseString()
    {
        if (position_ >= input_.size() ||
            input_[position_] != '"')
        {
            return false;
        }
        ++position_;

        while (position_ < input_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(input_[position_++]);

            if (character == '"') return true;
            if (character < 0x20U) return false;
            if (character != '\\') continue;

            if (position_ >= input_.size()) return false;
            const char escaped = input_[position_++];
            if (escaped == '"' || escaped == '\\' ||
                escaped == '/' || escaped == 'b' ||
                escaped == 'f' || escaped == 'n' ||
                escaped == 'r' || escaped == 't')
            {
                continue;
            }

            if (escaped != 'u' ||
                position_ + 4U > input_.size())
            {
                return false;
            }

            for (std::size_t index = 0; index < 4U; ++index)
            {
                const unsigned char hex =
                    static_cast<unsigned char>(
                        input_[position_ + index]);
                if (!std::isxdigit(hex)) return false;
            }
            position_ += 4U;
        }

        return false;
    }

    bool parseNumber()
    {
        const std::size_t start = position_;
        if (position_ < input_.size() &&
            input_[position_] == '-')
        {
            ++position_;
        }
        if (position_ >= input_.size()) return false;

        if (input_[position_] == '0')
        {
            ++position_;
        }
        else
        {
            if (input_[position_] < '1' ||
                input_[position_] > '9')
            {
                return false;
            }
            while (position_ < input_.size() &&
                   input_[position_] >= '0' &&
                   input_[position_] <= '9')
            {
                ++position_;
            }
        }

        if (position_ < input_.size() &&
            input_[position_] == '.')
        {
            ++position_;
            const std::size_t fractionStart = position_;
            while (position_ < input_.size() &&
                   input_[position_] >= '0' &&
                   input_[position_] <= '9')
            {
                ++position_;
            }
            if (position_ == fractionStart) return false;
        }

        if (position_ < input_.size() &&
            (input_[position_] == 'e' ||
             input_[position_] == 'E'))
        {
            ++position_;
            if (position_ < input_.size() &&
                (input_[position_] == '+' ||
                 input_[position_] == '-'))
            {
                ++position_;
            }
            const std::size_t exponentStart = position_;
            while (position_ < input_.size() &&
                   input_[position_] >= '0' &&
                   input_[position_] <= '9')
            {
                ++position_;
            }
            if (position_ == exponentStart) return false;
        }

        return position_ > start;
    }

    bool parseObject(std::size_t depth)
    {
        ++position_;
        skipWhitespace();
        if (position_ < input_.size() &&
            input_[position_] == '}')
        {
            ++position_;
            return true;
        }

        while (position_ < input_.size())
        {
            if (!parseString()) return false;
            skipWhitespace();
            if (position_ >= input_.size() ||
                input_[position_] != ':')
            {
                return false;
            }
            ++position_;
            skipWhitespace();
            if (!parseValue(depth)) return false;
            skipWhitespace();

            if (position_ >= input_.size()) return false;
            if (input_[position_] == '}')
            {
                ++position_;
                return true;
            }
            if (input_[position_] != ',') return false;
            ++position_;
            skipWhitespace();
        }
        return false;
    }

    bool parseArray(std::size_t depth)
    {
        ++position_;
        skipWhitespace();
        if (position_ < input_.size() &&
            input_[position_] == ']')
        {
            ++position_;
            return true;
        }

        while (position_ < input_.size())
        {
            if (!parseValue(depth)) return false;
            skipWhitespace();
            if (position_ >= input_.size()) return false;
            if (input_[position_] == ']')
            {
                ++position_;
                return true;
            }
            if (input_[position_] != ',') return false;
            ++position_;
            skipWhitespace();
        }
        return false;
    }

    const std::string& input_;
    std::size_t position_ = 0;
};

void skipAccountMutationWhitespace(
    const std::string& input,
    std::size_t& position)
{
    while (position < input.size() &&
           std::isspace(
               static_cast<unsigned char>(input[position])))
    {
        ++position;
    }
}

bool parseAccountMutationJsonString(
    const std::string& input,
    std::size_t& position,
    std::string& value)
{
    if (position >= input.size() ||
        input[position] != '"')
    {
        return false;
    }
    ++position;
    value.clear();

    while (position < input.size())
    {
        const unsigned char character =
            static_cast<unsigned char>(input[position++]);
        if (character == '"')
        {
            return true;
        }
        if (character < 0x20U)
        {
            return false;
        }
        if (character != '\\')
        {
            value.push_back(
                static_cast<char>(character));
            continue;
        }

        if (position >= input.size())
        {
            return false;
        }

        switch (input[position++])
        {
            case '"': value.push_back('"'); break;
            case '\\': value.push_back('\\'); break;
            case '/': value.push_back('/'); break;
            case 'b': value.push_back('\b'); break;
            case 'f': value.push_back('\f'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            default: return false;
        }
    }

    return false;
}

bool consumeAccountMutationLiteral(
    const std::string& input,
    std::size_t& position,
    const char* literal)
{
    const std::string value(literal);
    if (input.compare(
            position,
            value.size(),
            value) != 0)
    {
        return false;
    }

    position += value.size();
    return true;
}

bool parsePublicAccountMutationBody(
    const std::string& input,
    PublicAccountMutationKind& kind,
    std::string& displayName,
    bool& active)
{
    std::size_t position = 0U;
    skipAccountMutationWhitespace(input, position);
    if (position >= input.size() ||
        input[position++] != '{')
    {
        return false;
    }

    skipAccountMutationWhitespace(input, position);
    std::string key;
    if (!parseAccountMutationJsonString(
            input,
            position,
            key))
    {
        return false;
    }

    skipAccountMutationWhitespace(input, position);
    if (position >= input.size() ||
        input[position++] != ':')
    {
        return false;
    }
    skipAccountMutationWhitespace(input, position);

    if (key == "displayName")
    {
        if (!parseAccountMutationJsonString(
                input,
                position,
                displayName))
        {
            return false;
        }
        kind = PublicAccountMutationKind::displayName;
    }
    else if (key == "active")
    {
        if (consumeAccountMutationLiteral(
                input,
                position,
                "true"))
        {
            active = true;
        }
        else if (consumeAccountMutationLiteral(
                     input,
                     position,
                     "false"))
        {
            active = false;
        }
        else
        {
            return false;
        }
        kind = PublicAccountMutationKind::active;
    }
    else
    {
        return false;
    }

    skipAccountMutationWhitespace(input, position);
    if (position >= input.size() ||
        input[position++] != '}')
    {
        return false;
    }
    skipAccountMutationWhitespace(input, position);
    return position == input.size();
}

bool parsePublicAccountCreateBody(
    const std::string& input,
    std::string& loginName,
    std::string& displayName,
    std::string& password)
{
    std::size_t position = 0U;
    bool loginSeen = false;
    bool displaySeen = false;
    bool passwordSeen = false;

    skipAccountMutationWhitespace(input, position);
    if (position >= input.size() || input[position++] != '{')
    {
        return false;
    }

    while (true)
    {
        skipAccountMutationWhitespace(input, position);
        if (position < input.size() && input[position] == '}')
        {
            ++position;
            break;
        }

        std::string key;
        if (!parseAccountMutationJsonString(input, position, key))
        {
            return false;
        }

        skipAccountMutationWhitespace(input, position);
        if (position >= input.size() || input[position++] != ':')
        {
            return false;
        }
        skipAccountMutationWhitespace(input, position);

        std::string value;
        if (!parseAccountMutationJsonString(input, position, value))
        {
            return false;
        }

        if (key == "loginName" && !loginSeen)
        {
            loginSeen = true;
            loginName = std::move(value);
        }
        else if (key == "displayName" && !displaySeen)
        {
            displaySeen = true;
            displayName = std::move(value);
        }
        else if (key == "password" && !passwordSeen)
        {
            passwordSeen = true;
            password = std::move(value);
        }
        else
        {
            return false;
        }

        skipAccountMutationWhitespace(input, position);
        if (position >= input.size())
        {
            return false;
        }
        if (input[position] == ',')
        {
            ++position;
            continue;
        }
        if (input[position] == '}')
        {
            ++position;
            break;
        }
        return false;
    }

    skipAccountMutationWhitespace(input, position);
    return position == input.size() &&
        loginSeen && displaySeen && passwordSeen &&
        !loginName.empty() && !displayName.empty() && !password.empty();
}

bool parsePublicAccountGrantMutationBody(
    const std::string& input,
    std::string& permission,
    std::string& backendId,
    bool& active)
{
    std::size_t position = 0U;
    bool permissionSeen = false;
    bool backendSeen = false;
    bool activeSeen = false;

    skipAccountMutationWhitespace(input, position);
    if (position >= input.size() || input[position++] != '{')
    {
        return false;
    }

    while (true)
    {
        skipAccountMutationWhitespace(input, position);
        if (position < input.size() && input[position] == '}')
        {
            ++position;
            break;
        }

        std::string key;
        if (!parseAccountMutationJsonString(input, position, key))
        {
            return false;
        }

        skipAccountMutationWhitespace(input, position);
        if (position >= input.size() || input[position++] != ':')
        {
            return false;
        }
        skipAccountMutationWhitespace(input, position);

        if (key == "permission" && !permissionSeen)
        {
            permissionSeen = true;
            if (!parseAccountMutationJsonString(
                    input, position, permission))
            {
                return false;
            }
        }
        else if (key == "backendId" && !backendSeen)
        {
            backendSeen = true;
            if (!parseAccountMutationJsonString(
                    input, position, backendId))
            {
                return false;
            }
        }
        else if (key == "active" && !activeSeen)
        {
            activeSeen = true;
            if (consumeAccountMutationLiteral(
                    input, position, "true"))
            {
                active = true;
            }
            else if (consumeAccountMutationLiteral(
                         input, position, "false"))
            {
                active = false;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        skipAccountMutationWhitespace(input, position);
        if (position >= input.size())
        {
            return false;
        }
        if (input[position] == ',')
        {
            ++position;
            continue;
        }
        if (input[position] == '}')
        {
            ++position;
            break;
        }
        return false;
    }

    skipAccountMutationWhitespace(input, position);
    return position == input.size() &&
        permissionSeen && backendSeen && activeSeen &&
        !permission.empty() && !backendId.empty();
}

bool publicAccountRevision(
    const std::string& resourceRevision,
    std::uint64_t& revision)
{
    static const std::string Prefix = "account:";
    if (resourceRevision.compare(
            0U,
            Prefix.size(),
            Prefix) != 0 ||
        resourceRevision.size() <= Prefix.size())
    {
        return false;
    }

    std::uint64_t parsed = 0U;
    for (std::size_t index = Prefix.size();
         index < resourceRevision.size();
         ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(
                resourceRevision[index]);
        if (character < '0' || character > '9')
        {
            return false;
        }
        const std::uint64_t digit =
            static_cast<std::uint64_t>(
                character - '0');
        if (parsed >
            (std::numeric_limits<std::uint64_t>::max() -
             digit) / 10U)
        {
            return false;
        }
        parsed = parsed * 10U + digit;
    }

    if (parsed == 0U)
    {
        return false;
    }

    revision = parsed;
    return true;
}

bool publicGrantSetRevision(
    const std::string& resourceRevision)
{
    static const std::string Prefix = "grant-set:";
    if (resourceRevision.size() != Prefix.size() + 64U ||
        resourceRevision.compare(0U, Prefix.size(), Prefix) != 0)
    {
        return false;
    }

    return std::all_of(
        resourceRevision.begin() + Prefix.size(),
        resourceRevision.end(),
        [](unsigned char character)
        {
            return (character >= '0' && character <= '9') ||
                (character >= 'a' && character <= 'f');
        });
}

bool emptyJsonObject(const std::string& input)
{
    std::size_t position = 0;
    while (position < input.size() &&
           asciiWhitespace(input[position]))
    {
        ++position;
    }

    if (position >= input.size() || input[position] != '{')
        return false;
    ++position;

    while (position < input.size() &&
           asciiWhitespace(input[position]))
    {
        ++position;
    }

    if (position >= input.size() || input[position] != '}')
        return false;
    ++position;

    while (position < input.size() &&
           asciiWhitespace(input[position]))
    {
        ++position;
    }

    return position == input.size();
}

void addRequestContextHeaders(
    ApiResponse& response,
    const std::string& requestId,
    const std::string& correlationId)
{
    if (!requestId.empty())
    {
        response.headers["X-Request-ID"] = requestId;
    }

    if (!correlationId.empty())
    {
        response.headers["X-Correlation-ID"] = correlationId;
    }
}

void addPublicSuccessHeaders(
    ApiResponse& response,
    const std::string& requestId,
    const std::string& correlationId)
{
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    addRequestContextHeaders(
        response,
        requestId,
        correlationId);
}

ApiResponse jsonResponse(
    const std::string& body,
    const std::string& requestId,
    const std::string& correlationId)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json; charset=utf-8";
    addPublicSuccessHeaders(
        response,
        requestId,
        correlationId);
    response.body = body;
    return response;
}

ApiResponse problemResponse(
    int statusCode,
    const std::string& code,
    const std::string& title,
    const std::string& detail,
    const std::string& instance,
    const std::string& requestId,
    const std::string& correlationId)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = PublicProblemDetails::contentType();
    addPublicSuccessHeaders(
        response,
        requestId,
        correlationId);

    PublicProblemDetails problem;
    problem.statusCode = statusCode;
    problem.code = code;
    problem.title = title;
    problem.detail = detail;
    problem.instance = instance;
    problem.requestId = requestId;
    problem.correlationId = correlationId;
    response.body = problem.serialize();
    return response;
}

ApiResponse notFoundProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        404,
        "not_found",
        "Resource not found",
        "The requested public API resource is not available.",
        path,
        requestId,
        correlationId);
}

ApiResponse invalidRequestProblem(
    const std::string& path,
    const std::string& detail,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        400,
        "invalid_request",
        "Invalid request",
        detail,
        path,
        requestId,
        correlationId);
}

ApiResponse unauthorizedProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        401,
        "unauthorized",
        "Authentication required",
        "Authentication is required for this public API resource.",
        path,
        requestId,
        correlationId);
}

ApiResponse serviceUnavailableProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        503,
        "service_unavailable",
        "Service unavailable",
        "The requested public API resource is temporarily unavailable.",
        path,
        requestId,
        correlationId);
}

ApiResponse backendUnavailableProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        503,
        "backend_unavailable",
        "Backend unavailable",
        "No authorized requested backend can currently supply this collection.",
        path,
        requestId,
        correlationId);
}

ApiResponse cursorExpiredProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        409,
        "cursor_expired",
        "Collection cursor expired",
        "The authorized backend scope changed and this traversal cannot continue.",
        path,
        requestId,
        correlationId);
}

ApiResponse methodNotAllowedProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& allow = "GET")
{
    ApiResponse response = problemResponse(
        405,
        "method_not_allowed",
        "Method not allowed",
        "The requested public API resource does not support this method.",
        path,
        requestId,
        correlationId);
    response.headers["Allow"] = allow;
    return response;
}

ApiResponse publicTimerCreateProblem(
    int statusCode,
    const std::string& code,
    const std::string& title,
    const std::string& detail,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        statusCode,
        code,
        title,
        detail,
        path,
        requestId,
        correlationId);
}

ApiResponse contractRoot(
    const bool authenticated,
    const std::string& requestId,
    const std::string& correlationId)
{
    return jsonResponse(
        std::string("{\"apiVersion\":\"v1\",\"serverVersion\":\"")
        + VdrSuiteServerBuildIdentity::ServerVersion
        + "\",\"supportedApiMajors\":[\"v1\"],"
          "\"compatibilityPolicy\":{\"version\":1,"
          "\"responseEvolution\":\"additive\","
          "\"breakingChanges\":\"new-major\","
          "\"unknownRequestFields\":\"reject\","
          "\"legacyUnversioned\":\"transition\"},"
          "\"authentication\":{\"authenticated\":"
        + (authenticated ? "true" : "false")
        + "},\"links\":{\"self\":\"/api/v1\",\"capabilities\":\"/api/v1/capabilities\",\"backends\":\"/api/v1/backends\",\"accounts\":\"/api/v1/accounts\"}}",
        requestId,
        correlationId);
}

ApiResponse platformCapabilities(
    const bool operationReadAvailable,
    const bool timerAssignmentReadAvailable,
    const bool timerCreateAdmissionAvailable,
    const bool backendCollectionAvailable,
    const bool accountCollectionAvailable,
    const bool accountMutationAvailable,
    const bool accountCreateAvailable,
    const bool accountGrantAdministrationAvailable,
    const std::string& requestId,
    const std::string& correlationId)
{
    return jsonResponse(
        "{\"apiVersion\":\"v1\",\"capabilities\":["
        "{\"id\":\"public-api.contract-root\",\"version\":1,\"availability\":\"available\"},"
        "{\"id\":\"public-api.durable-operations-read\",\"version\":1,\"availability\":\"" +
        std::string(operationReadAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.timer-assignments-read\",\"version\":1,\"availability\":\"" +
        std::string(timerAssignmentReadAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.timer-create-admission\",\"version\":1,\"availability\":\"" +
        std::string(timerCreateAdmissionAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.backends-read\",\"version\":1,\"availability\":\"" +
        std::string(backendCollectionAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-read\",\"version\":1,\"availability\":\"" +
        std::string(accountCollectionAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-lifecycle-mutation\",\"version\":1,\"availability\":\"" +
        std::string(accountMutationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-create\",\"version\":1,\"availability\":\"" +
        std::string(accountCreateAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-grants-administration\",\"version\":1,\"availability\":\"" +
        std::string(accountGrantAdministrationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.compatibility-policy\",\"version\":1,\"availability\":\"available\"},"
        "{\"id\":\"public-api.deprecation-metadata\",\"version\":1,\"availability\":\"available\"}"
        "],\"compatibility\":{\"policyVersion\":1,"
        "\"supportedApiMajors\":[\"v1\"],"
        "\"responseEvolution\":\"additive\","
        "\"breakingChanges\":\"new-major\","
        "\"unknownRequestFields\":\"reject\","
        "\"legacyUnversioned\":\"transition\","
        "\"lifecycle\":[\"supported\",\"deprecated\",\"sunset-announced\",\"removed\"],"
        "\"deprecationHeaders\":[\"Deprecation\",\"Sunset\",\"Link\"],"
        "\"deprecatedAliases\":[]},"
        "\"links\":{\"self\":\"/api/v1/capabilities\",\"root\":\"/api/v1\"}}",
        requestId,
        correlationId);
}

ApiResponse publicOperationResponse(
    const PublicOperationResource& operation,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& ifNoneMatch)
{
    const std::string entityTag =
        vdrsuite::http::publicStrongEntityTag(
            operation.resourceRevision);

    if (entityTag.empty())
    {
        return serviceUnavailableProblem(
            path,
            requestId,
            correlationId);
    }

    const vdrsuite::http::PublicEntityTagConditionResult condition =
        vdrsuite::http::publicEvaluateIfNoneMatch(
            ifNoneMatch,
            entityTag);

    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::malformed)
    {
        return invalidRequestProblem(
            path,
            "If-None-Match is not a valid entity-tag condition.",
            requestId,
            correlationId);
    }

    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::matched)
    {
        ApiResponse response;
        response.statusCode = 304;
        response.contentType = "application/json; charset=utf-8";
        addPublicSuccessHeaders(
            response,
            requestId,
            correlationId);
        response.headers["ETag"] = entityTag;
        return response;
    }

    ApiResponse response = jsonResponse(
        "{\"operationId\":\"" + jsonEscape(operation.operationId) +
        "\",\"state\":\"" + jsonEscape(operation.state) +
        "\",\"backendId\":\"" + jsonEscape(operation.backendId) +
        "\",\"links\":{\"self\":\"" + jsonEscape(path) + "\"}}",
        requestId,
        correlationId);
    response.headers["ETag"] = entityTag;
    return response;
}

ApiResponse publicTimerAssignmentResponse(
    const PublicTimerAssignmentRevisionResource& assignment,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& ifNoneMatch)
{
    const std::string entityTag =
        vdrsuite::http::publicStrongEntityTag(
            assignment.resourceRevision);

    if (entityTag.empty())
    {
        return serviceUnavailableProblem(
            path,
            requestId,
            correlationId);
    }

    const vdrsuite::http::PublicEntityTagConditionResult condition =
        vdrsuite::http::publicEvaluateIfNoneMatch(
            ifNoneMatch,
            entityTag);

    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::malformed)
    {
        return invalidRequestProblem(
            path,
            "If-None-Match is not a valid entity-tag condition.",
            requestId,
            correlationId);
    }

    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::matched)
    {
        ApiResponse response;
        response.statusCode = 304;
        response.contentType = "application/json; charset=utf-8";
        addPublicSuccessHeaders(
            response,
            requestId,
            correlationId);
        response.headers["ETag"] = entityTag;
        return response;
    }

    const std::string self =
        path + "?backend=" + assignment.backendId;
    ApiResponse response = jsonResponse(
        "{\"timerAssignmentId\":\"" +
        jsonEscape(assignment.timerAssignmentId) +
        "\",\"backendId\":\"" +
        jsonEscape(assignment.backendId) +
        "\",\"links\":{\"self\":\"" +
        jsonEscape(self) + "\"}}",
        requestId,
        correlationId);
    response.headers["ETag"] = entityTag;
    return response;
}

ApiResponse publicTimerAssignmentCollectionResponse(
    const PublicTimerAssignmentCollectionResult& page,
    const PublicTimerAssignmentCollectionQuery& query,
    const std::string& actorRef,
    const std::string& backendId,
    const std::string& requestId,
    const std::string& correlationId)
{
    const std::string nextCursor =
        page.hasMore && !page.assignments.empty()
            ? publicTimerAssignmentCursor(
                backendId,
                page.assignments.back().timerAssignmentId)
            : std::string();

    if (page.hasMore && nextCursor.empty())
        return serviceUnavailableProblem(
            PublicTimerAssignmentCollectionPath,
            requestId,
            correlationId);

    const std::string self =
        publicTimerAssignmentCollectionTarget(
            backendId,
            query.limit,
            query.cursor);
    const std::string next =
        nextCursor.empty()
            ? std::string()
            : publicTimerAssignmentCollectionTarget(
                backendId,
                query.limit,
                nextCursor);

    std::string body = "{\"items\":[";
    for (std::size_t index = 0U;
         index < page.assignments.size();
         ++index)
    {
        if (index > 0U) body += ",";
        const PublicTimerAssignmentCollectionItem& assignment =
            page.assignments[index];
        const std::string itemSelf =
            std::string(PublicTimerAssignmentCollectionPath) +
            "/" + assignment.timerAssignmentId +
            "?backend=" + assignment.backendId;
        body +=
            "{\"timerAssignmentId\":\"" +
            jsonEscape(assignment.timerAssignmentId) +
            "\",\"backendId\":\"" +
            jsonEscape(assignment.backendId) +
            "\",\"links\":{\"self\":\"" +
            jsonEscape(itemSelf) + "\"}}";
    }

    body +=
        "],\"page\":{\"limit\":" +
        std::to_string(query.limit) +
        ",\"nextCursor\":";
    body += nextCursor.empty()
        ? "null"
        : "\"" + jsonEscape(nextCursor) + "\"";
    body +=
        ",\"hasMore\":" +
        std::string(page.hasMore ? "true" : "false") +
        "},\"meta\":{\"partial\":false},"
        "\"links\":{\"self\":\"" +
        jsonEscape(self) + "\",\"next\":";
    body += next.empty()
        ? "null"
        : "\"" + jsonEscape(next) + "\"";
    body += "}}";

    return jsonResponse(
        body,
        requestId,
        correlationId);
}

ApiResponse publicBackendCollectionResponse(
    const PublicBackendCollectionResult& page,
    const PublicBackendCollectionQuery& query,
    const std::string& authorizationScope,
    const std::string& requestId,
    const std::string& correlationId)
{
    const std::string nextCursor =
        page.hasMore && !page.backends.empty()
            ? publicBackendCursor(
                authorizationScope,
                page.backends.back().backendId)
            : std::string();

    if (page.hasMore && nextCursor.empty())
        return serviceUnavailableProblem(
            PublicBackendCollectionPath,
            requestId,
            correlationId);

    const std::string self =
        publicBackendCollectionTarget(
            query.limit,
            query.cursor);
    const std::string next =
        nextCursor.empty()
            ? std::string()
            : publicBackendCollectionTarget(
                query.limit,
                nextCursor);

    std::string body = "{\"items\":[";
    for (std::size_t index = 0U; index < page.backends.size(); ++index)
    {
        if (index > 0U) body += ",";
        const PublicBackendCollectionItem& backend = page.backends[index];
        body +=
            "{\"backendId\":\"" + jsonEscape(backend.backendId) +
            "\",\"name\":\"" + jsonEscape(backend.name) +
            "\",\"enabled\":" + std::string(backend.enabled ? "true" : "false") +
            ",\"online\":" + std::string(backend.online ? "true" : "false") +
            "}";
    }

    body +=
        "],\"page\":{\"limit\":" +
        std::to_string(query.limit) +
        ",\"nextCursor\":";
    body += nextCursor.empty()
        ? "null"
        : "\"" + jsonEscape(nextCursor) + "\"";
    body +=
        ",\"hasMore\":" +
        std::string(page.hasMore ? "true" : "false") +
        "},\"meta\":{\"partial\":false},"
        "\"links\":{\"self\":\"" +
        jsonEscape(self) + "\",\"next\":";
    body += next.empty()
        ? "null"
        : "\"" + jsonEscape(next) + "\"";
    body += "}}";

    return jsonResponse(
        body,
        requestId,
        correlationId);
}

ApiResponse publicAccountResponse(
    const PublicAccountResource& account,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& ifNoneMatch)
{
    const std::string entityTag =
        vdrsuite::http::publicStrongEntityTag(
            account.resourceRevision);

    if (entityTag.empty())
    {
        return serviceUnavailableProblem(
            path,
            requestId,
            correlationId);
    }

    const vdrsuite::http::PublicEntityTagConditionResult condition =
        vdrsuite::http::publicEvaluateIfNoneMatch(
            ifNoneMatch,
            entityTag);

    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::malformed)
    {
        return invalidRequestProblem(
            path,
            "If-None-Match is not a valid entity-tag condition.",
            requestId,
            correlationId);
    }

    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::matched)
    {
        ApiResponse response;
        response.statusCode = 304;
        response.contentType = "application/json; charset=utf-8";
        addPublicSuccessHeaders(
            response,
            requestId,
            correlationId);
        response.headers["ETag"] = entityTag;
        return response;
    }

    ApiResponse response = jsonResponse(
        "{\"accountId\":\"" + jsonEscape(account.accountId) +
        "\",\"actorId\":\"" + jsonEscape(account.actorId) +
        "\",\"displayName\":\"" + jsonEscape(account.displayName) +
        "\",\"active\":" +
        std::string(account.active ? "true" : "false") +
        ",\"links\":{\"self\":\"" + jsonEscape(path) + "\"}}",
        requestId,
        correlationId);
    response.headers["ETag"] = entityTag;
    return response;
}

ApiResponse publicAccountGrantSetResponse(
    const PublicAccountGrantSetResource& grantSet,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& ifNoneMatch)
{
    const std::string entityTag =
        vdrsuite::http::publicStrongEntityTag(
            grantSet.resourceRevision);
    if (entityTag.empty())
    {
        return serviceUnavailableProblem(
            path, requestId, correlationId);
    }

    const vdrsuite::http::PublicEntityTagConditionResult condition =
        vdrsuite::http::publicEvaluateIfNoneMatch(
            ifNoneMatch,
            entityTag);
    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::malformed)
    {
        return invalidRequestProblem(
            path,
            "If-None-Match is not a valid entity-tag condition.",
            requestId,
            correlationId);
    }
    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::matched)
    {
        ApiResponse response;
        response.statusCode = 304;
        response.contentType = "application/json; charset=utf-8";
        addPublicSuccessHeaders(
            response, requestId, correlationId);
        response.headers["ETag"] = entityTag;
        return response;
    }

    std::string body =
        "{\"accountId\":\"" +
        jsonEscape(grantSet.accountId) +
        "\",\"actorId\":\"" +
        jsonEscape(grantSet.actorId) +
        "\",\"items\":[";
    for (std::size_t index = 0U;
         index < grantSet.grants.size();
         ++index)
    {
        if (index > 0U) body += ",";
        const PublicAccountGrantItem& grant =
            grantSet.grants[index];
        body +=
            "{\"permission\":\"" +
            jsonEscape(grant.permission) +
            "\",\"backendId\":\"" +
            jsonEscape(grant.backendId) +
            "\"}";
    }
    body +=
        "],\"links\":{\"self\":\"" +
        jsonEscape(path) +
        "\"}}";

    ApiResponse response = jsonResponse(
        body,
        requestId,
        correlationId);
    response.headers["ETag"] = entityTag;
    return response;
}

ApiResponse publicAccountCollectionResponse(
    const PublicAccountCollectionResult& page,
    const PublicAccountCollectionQuery& query,
    const std::string& requestId,
    const std::string& correlationId)
{
    const std::string nextCursor =
        page.hasMore && !page.accounts.empty()
            ? publicAccountCursor(page.accounts.back().accountId)
            : std::string();

    if (page.hasMore && nextCursor.empty())
        return serviceUnavailableProblem(
            PublicAccountCollectionPath,
            requestId,
            correlationId);

    const std::string self =
        publicAccountCollectionTarget(query.limit, query.cursor);
    const std::string next =
        nextCursor.empty()
            ? std::string()
            : publicAccountCollectionTarget(query.limit, nextCursor);

    std::string body = "{\"items\":[";
    for (std::size_t index = 0U; index < page.accounts.size(); ++index)
    {
        if (index > 0U) body += ",";
        const PublicAccountCollectionItem& account =
            page.accounts[index];
        body +=
            "{\"accountId\":\"" + jsonEscape(account.accountId) +
            "\",\"actorId\":\"" + jsonEscape(account.actorId) +
            "\",\"displayName\":\"" + jsonEscape(account.displayName) +
            "\",\"active\":" +
            std::string(account.active ? "true" : "false") +
            "}";
    }

    body +=
        "],\"page\":{\"limit\":" +
        std::to_string(query.limit) +
        ",\"nextCursor\":";
    body += nextCursor.empty()
        ? "null"
        : "\"" + jsonEscape(nextCursor) + "\"";
    body +=
        ",\"hasMore\":" +
        std::string(page.hasMore ? "true" : "false") +
        "},\"meta\":{\"partial\":false},"
        "\"links\":{\"self\":\"" +
        jsonEscape(self) + "\",\"next\":";
    body += next.empty()
        ? "null"
        : "\"" + jsonEscape(next) + "\"";
    body += "}}";

    return jsonResponse(
        body,
        requestId,
        correlationId);
}

ApiResponse publicChannelCollectionResponse(
    const PublicChannelCollectionResult& page,
    const PublicChannelCollectionQuery& query,
    const std::vector<std::string>& backendIds,
    const std::string& backendScope,
    const std::string& requestId,
    const std::string& correlationId)
{
    const std::string nextCursor =
        page.hasMore && !page.channels.empty()
            ? publicChannelCursor(
                backendScope,
                page.channels.back().backendId,
                page.channels.back().channelId)
            : std::string();

    if (page.hasMore && nextCursor.empty())
        return serviceUnavailableProblem(
            PublicChannelCollectionPath,
            requestId,
            correlationId);

    const std::string self =
        publicChannelCollectionTarget(
            backendIds,
            query.limit,
            query.cursor);
    const std::string next =
        nextCursor.empty()
            ? std::string()
            : publicChannelCollectionTarget(
                backendIds,
                query.limit,
                nextCursor);

    bool partial = false;
    std::string body = "{\"items\":[";
    for (std::size_t index = 0U; index < page.channels.size(); ++index)
    {
        if (index > 0U) body += ",";
        const PublicChannelCollectionItem& channel = page.channels[index];
        body +=
            "{\"backendId\":\"" + jsonEscape(channel.backendId) +
            "\",\"channelId\":\"" + jsonEscape(channel.channelId) +
            "\",\"channelNumber\":" + std::to_string(channel.channelNumber) +
            ",\"name\":\"" + jsonEscape(channel.name) +
            "\",\"provider\":\"" + jsonEscape(channel.provider) +
            "\",\"groupName\":\"" + jsonEscape(channel.groupName) +
            "\",\"radio\":" + std::string(channel.radio ? "true" : "false") +
            ",\"encrypted\":" + std::string(channel.encrypted ? "true" : "false") +
            ",\"enabled\":" + std::string(channel.enabled ? "true" : "false") +
            "}";
    }

    body += "],\"page\":{\"limit\":" +
        std::to_string(query.limit) + ",\"nextCursor\":";
    body += nextCursor.empty()
        ? "null"
        : "\"" + jsonEscape(nextCursor) + "\"";
    body += ",\"hasMore\":" +
        std::string(page.hasMore ? "true" : "false") +
        "},\"meta\":{\"partial\":";

    for (const PublicChannelCollectionSource& source : page.sources)
    {
        if (source.state != "ok")
        {
            partial = true;
            break;
        }
    }
    body += partial ? "true" : "false";
    body += ",\"sources\":[";
    for (std::size_t index = 0U; index < page.sources.size(); ++index)
    {
        if (index > 0U) body += ",";
        const PublicChannelCollectionSource& source = page.sources[index];
        body +=
            "{\"backendId\":\"" + jsonEscape(source.backendId) +
            "\",\"state\":\"" + jsonEscape(source.state) + "\"";
        if (!source.code.empty())
            body += ",\"code\":\"" + jsonEscape(source.code) + "\"";
        body += "}";
    }
    body += "]},\"links\":{\"self\":\"" +
        jsonEscape(self) + "\",\"next\":";
    body += next.empty()
        ? "null"
        : "\"" + jsonEscape(next) + "\"";
    body += "}}";

    return jsonResponse(body, requestId, correlationId);
}

}

PublicApiRuntime& PublicApiRuntime::instance()
{
    static PublicApiRuntime runtime;
    return runtime;
}

void PublicApiRuntime::registerOperationLookup(
    OperationLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        operationLookupMutex_);
    operationLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetOperationLookup()
{
    std::lock_guard<std::mutex> lock(
        operationLookupMutex_);
    operationLookup_ = OperationLookup{};
}

bool PublicApiRuntime::operationLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        operationLookupMutex_);
    return static_cast<bool>(operationLookup_);
}

void PublicApiRuntime::registerTimerAssignmentLookup(
    TimerAssignmentLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentLookupMutex_);
    timerAssignmentLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetTimerAssignmentLookup()
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentLookupMutex_);
    timerAssignmentLookup_ = TimerAssignmentLookup{};
}

bool PublicApiRuntime::timerAssignmentLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentLookupMutex_);
    return static_cast<bool>(timerAssignmentLookup_);
}

void PublicApiRuntime::registerTimerCreateAdmission(
    TimerCreateAdmission admission)
{
    std::lock_guard<std::mutex> lock(
        timerCreateAdmissionMutex_);
    timerCreateAdmission_ = std::move(admission);
}

void PublicApiRuntime::resetTimerCreateAdmission()
{
    std::lock_guard<std::mutex> lock(
        timerCreateAdmissionMutex_);
    timerCreateAdmission_ = TimerCreateAdmission{};
}

bool PublicApiRuntime::timerCreateAdmissionConfigured() const
{
    std::lock_guard<std::mutex> lock(
        timerCreateAdmissionMutex_);
    return static_cast<bool>(timerCreateAdmission_);
}

void PublicApiRuntime::registerTimerAssignmentCollectionLookup(
    TimerAssignmentCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentCollectionLookupMutex_);
    timerAssignmentCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetTimerAssignmentCollectionLookup()
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentCollectionLookupMutex_);
    timerAssignmentCollectionLookup_ = {};
}

bool PublicApiRuntime::timerAssignmentCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentCollectionLookupMutex_);
    return static_cast<bool>(timerAssignmentCollectionLookup_);
}

PublicTimerAssignmentCollectionResult
PublicApiRuntime::lookupTimerAssignmentCollection(
    const PublicTimerAssignmentCollectionRequest& request) const
{
    TimerAssignmentCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(
            timerAssignmentCollectionLookupMutex_);
        lookup = timerAssignmentCollectionLookup_;
    }
    if (!lookup) return {};
    return lookup(request);
}

void PublicApiRuntime::registerBackendCollectionLookup(
    BackendCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        backendCollectionLookupMutex_);
    backendCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetBackendCollectionLookup()
{
    std::lock_guard<std::mutex> lock(
        backendCollectionLookupMutex_);
    backendCollectionLookup_ = {};
}

bool PublicApiRuntime::backendCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        backendCollectionLookupMutex_);
    return static_cast<bool>(backendCollectionLookup_);
}

PublicBackendCollectionResult
PublicApiRuntime::lookupBackendCollection(
    const PublicBackendCollectionRequest& request) const
{
    BackendCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(
            backendCollectionLookupMutex_);
        lookup = backendCollectionLookup_;
    }
    if (!lookup) return {};
    return lookup(request);
}

void PublicApiRuntime::registerAccountCollectionLookup(
    AccountCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountCollectionLookupMutex_);
    accountCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountCollectionLookup()
{
    std::lock_guard<std::mutex> lock(
        accountCollectionLookupMutex_);
    accountCollectionLookup_ = {};
}

bool PublicApiRuntime::accountCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountCollectionLookupMutex_);
    return static_cast<bool>(accountCollectionLookup_);
}

PublicAccountCollectionResult
PublicApiRuntime::lookupAccountCollection(
    const PublicAccountCollectionRequest& request) const
{
    AccountCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(
            accountCollectionLookupMutex_);
        lookup = accountCollectionLookup_;
    }
    if (!lookup) return {};
    return lookup(request);
}

void PublicApiRuntime::registerAccountLookup(
    AccountLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountLookupMutex_);
    accountLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountLookup()
{
    std::lock_guard<std::mutex> lock(
        accountLookupMutex_);
    accountLookup_ = {};
}

bool PublicApiRuntime::accountLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountLookupMutex_);
    return static_cast<bool>(accountLookup_);
}

PublicAccountLookupResult
PublicApiRuntime::lookupAccount(
    const std::string& accountId) const
{
    AccountLookup lookup;
    {
        std::lock_guard<std::mutex> lock(
            accountLookupMutex_);
        lookup = accountLookup_;
    }
    if (!lookup) return {};
    return lookup(accountId);
}

void PublicApiRuntime::registerAccountMutation(
    AccountMutation mutation)
{
    std::lock_guard<std::mutex> lock(
        accountMutationMutex_);
    accountMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetAccountMutation()
{
    std::lock_guard<std::mutex> lock(
        accountMutationMutex_);
    accountMutation_ = {};
}

bool PublicApiRuntime::accountMutationConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountMutationMutex_);
    return static_cast<bool>(accountMutation_);
}

void PublicApiRuntime::registerAccountCreate(
    AccountCreate create)
{
    std::lock_guard<std::mutex> lock(
        accountCreateMutex_);
    accountCreate_ = std::move(create);
}

void PublicApiRuntime::resetAccountCreate()
{
    std::lock_guard<std::mutex> lock(
        accountCreateMutex_);
    accountCreate_ = {};
}

bool PublicApiRuntime::accountCreateConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountCreateMutex_);
    return static_cast<bool>(accountCreate_);
}

void PublicApiRuntime::registerAccountGrantLookup(
    AccountGrantLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountGrantLookupMutex_);
    accountGrantLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountGrantLookup()
{
    std::lock_guard<std::mutex> lock(
        accountGrantLookupMutex_);
    accountGrantLookup_ = {};
}

bool PublicApiRuntime::accountGrantLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountGrantLookupMutex_);
    return static_cast<bool>(accountGrantLookup_);
}

void PublicApiRuntime::registerAccountGrantMutation(
    AccountGrantMutation mutation)
{
    std::lock_guard<std::mutex> lock(
        accountGrantMutationMutex_);
    accountGrantMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetAccountGrantMutation()
{
    std::lock_guard<std::mutex> lock(
        accountGrantMutationMutex_);
    accountGrantMutation_ = {};
}

bool PublicApiRuntime::accountGrantMutationConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountGrantMutationMutex_);
    return static_cast<bool>(accountGrantMutation_);
}

void PublicApiRuntime::registerChannelCollectionLookup(
    ChannelCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        channelCollectionLookupMutex_);
    channelCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetChannelCollectionLookup()
{
    std::lock_guard<std::mutex> lock(
        channelCollectionLookupMutex_);
    channelCollectionLookup_ = {};
}

bool PublicApiRuntime::channelCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        channelCollectionLookupMutex_);
    return static_cast<bool>(channelCollectionLookup_);
}

PublicChannelCollectionResult
PublicApiRuntime::lookupChannelCollection(
    const PublicChannelCollectionRequest& request) const
{
    ChannelCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(
            channelCollectionLookupMutex_);
        lookup = channelCollectionLookup_;
    }
    if (!lookup) return {};
    return lookup(request);
}

PublicTimerAssignmentLookupResult
PublicApiRuntime::lookupTimerAssignment(
    const std::string& timerAssignmentId,
    const std::string& backendId) const
{
    TimerAssignmentLookup lookup;

    {
        std::lock_guard<std::mutex> lock(
            timerAssignmentLookupMutex_);
        lookup = timerAssignmentLookup_;
    }

    if (!lookup)
    {
        return {};
    }

    return lookup(timerAssignmentId, backendId);
}

PublicOperationLookupResult PublicApiRuntime::lookupOperation(
    const std::string& operationId,
    const std::string& actorRef) const
{
    OperationLookup lookup;

    {
        std::lock_guard<std::mutex> lock(
            operationLookupMutex_);
        lookup = operationLookup_;
    }

    if (!lookup)
    {
        return {};
    }

    return lookup(operationId, actorRef);
}

bool PublicApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    const std::string& actorRef,
    const std::string& requestId,
    const std::string& correlationId,
    ApiResponse& response,
    const std::string& ifNoneMatch,
    const std::string& authorizedBackendId,
    const std::vector<std::string>& authorizedBackendIds) const
{
    const std::string path = requestPath(requestTarget);

    if (path == "/api/v1")
    {
        response = contractRoot(
            !actorRef.empty(),
            requestId,
            correlationId);
        return true;
    }

    if (path == "/api/v1/capabilities")
    {
        response = platformCapabilities(
            operationLookupConfigured(),
            timerAssignmentLookupConfigured(),
            timerCreateAdmissionConfigured(),
            backendCollectionLookupConfigured(),
            accountCollectionLookupConfigured(),
            accountMutationConfigured(),
            accountCreateConfigured(),
            accountGrantLookupConfigured() &&
                accountGrantMutationConfigured(),
            requestId,
            correlationId);
        return true;
    }

    std::string operationId;
    if (publicOperationPath(path, operationId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        const PublicOperationLookupResult found =
            lookupOperation(operationId, actorRef);

        switch (found.status)
        {
            case PublicOperationLookupStatus::ok:
                if (found.operation.operationId != operationId ||
                    found.operation.state.empty() ||
                    found.operation.backendId.empty() ||
                    found.operation.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                }
                else
                {
                    response = publicOperationResponse(
                        found.operation,
                        path,
                        requestId,
                        correlationId,
                        ifNoneMatch);
                }
                return true;

            case PublicOperationLookupStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The operation identifier is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicOperationLookupStatus::notFound:
                response = notFoundProblem(
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicOperationLookupStatus::unavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
    }

    std::string accountId;
    if (publicAccountPath(path, accountId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path, requestId, correlationId);
            return true;
        }

        const PublicAccountLookupResult found =
            lookupAccount(accountId);

        switch (found.status)
        {
            case PublicAccountLookupStatus::ok:
                if (found.account.accountId != accountId ||
                    found.account.actorId.empty() ||
                    found.account.displayName.empty() ||
                    found.account.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                }
                else
                {
                    response = publicAccountResponse(
                        found.account,
                        path,
                        requestId,
                        correlationId,
                        ifNoneMatch);
                }
                return true;

            case PublicAccountLookupStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The Account identifier is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicAccountLookupStatus::notFound:
                response = notFoundProblem(
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountLookupStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (path == PublicBackendCollectionPath)
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path, requestId, correlationId);
            return true;
        }

        std::vector<std::string> normalizedScopes =
            authorizedBackendIds;
        std::sort(normalizedScopes.begin(), normalizedScopes.end());
        if (std::adjacent_find(
                normalizedScopes.begin(),
                normalizedScopes.end()) != normalizedScopes.end())
        {
            response = invalidRequestProblem(
                path,
                "Duplicate authorized backend scopes are not allowed.",
                requestId,
                correlationId);
            return true;
        }

        PublicBackendCollectionQuery query;
        if (!parsePublicBackendCollectionQuery(
                requestTarget,
                query))
        {
            response = invalidRequestProblem(
                path,
                "The Backend collection query is invalid.",
                requestId,
                correlationId);
            return true;
        }

        const std::string authorizationScope =
            normalizedPublicBackendAuthorizationScope(
                normalizedScopes);

        std::string afterBackendId;
        if (!query.cursor.empty())
        {
            const PublicBackendCursorDecodeStatus cursorStatus =
                decodePublicBackendCursor(
                    query.cursor,
                    authorizationScope,
                    afterBackendId);
            if (cursorStatus ==
                PublicBackendCursorDecodeStatus::scopeMismatch)
            {
                response = cursorExpiredProblem(
                    path, requestId, correlationId);
                return true;
            }
            if (cursorStatus !=
                PublicBackendCursorDecodeStatus::ok)
            {
                response = invalidRequestProblem(
                    path,
                    "The collection cursor is invalid for this authorization scope.",
                    requestId,
                    correlationId);
                return true;
            }
        }

        PublicBackendCollectionRequest request;
        request.authorizedBackendIds = normalizedScopes;
        request.afterBackendId = afterBackendId;
        request.limit = query.limit;

        const PublicBackendCollectionResult page =
            lookupBackendCollection(request);

        switch (page.status)
        {
            case PublicBackendCollectionStatus::ok:
            {
                if (page.backends.size() > query.limit ||
                    (page.hasMore &&
                     page.backends.size() != query.limit))
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }

                const bool wildcard =
                    std::binary_search(
                        normalizedScopes.begin(),
                        normalizedScopes.end(),
                        std::string("*"));
                std::string previousBackend = afterBackendId;
                for (const PublicBackendCollectionItem& backend :
                     page.backends)
                {
                    const bool authorized =
                        wildcard ||
                        std::binary_search(
                            normalizedScopes.begin(),
                            normalizedScopes.end(),
                            backend.backendId);
                    if (backend.backendId.empty() ||
                        backend.name.empty() ||
                        !authorized ||
                        (!previousBackend.empty() &&
                         backend.backendId <= previousBackend))
                    {
                        response = serviceUnavailableProblem(
                            path, requestId, correlationId);
                        return true;
                    }
                    previousBackend = backend.backendId;
                }

                response = publicBackendCollectionResponse(
                    page,
                    query,
                    authorizationScope,
                    requestId,
                    correlationId);
                return true;
            }

            case PublicBackendCollectionStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (path == PublicAccountCollectionPath)
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path, requestId, correlationId);
            return true;
        }

        PublicAccountCollectionQuery query;
        if (!parsePublicAccountCollectionQuery(
                requestTarget,
                query))
        {
            response = invalidRequestProblem(
                path,
                "The Account collection query is invalid.",
                requestId,
                correlationId);
            return true;
        }

        std::string afterAccountId;
        if (!query.cursor.empty() &&
            !decodePublicAccountCursor(
                query.cursor,
                afterAccountId))
        {
            response = invalidRequestProblem(
                path,
                "The Account collection cursor is invalid.",
                requestId,
                correlationId);
            return true;
        }

        PublicAccountCollectionRequest request;
        request.afterAccountId = afterAccountId;
        request.limit = query.limit;

        const PublicAccountCollectionResult page =
            lookupAccountCollection(request);

        switch (page.status)
        {
            case PublicAccountCollectionStatus::ok:
            {
                if (page.accounts.size() > query.limit ||
                    (page.hasMore &&
                     page.accounts.size() != query.limit))
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }

                std::string previousAccount = afterAccountId;
                for (const PublicAccountCollectionItem& account :
                     page.accounts)
                {
                    if (account.accountId.empty() ||
                        account.actorId.empty() ||
                        account.displayName.empty() ||
                        (!previousAccount.empty() &&
                         account.accountId <= previousAccount))
                    {
                        response = serviceUnavailableProblem(
                            path, requestId, correlationId);
                        return true;
                    }
                    previousAccount = account.accountId;
                }

                response = publicAccountCollectionResponse(
                    page,
                    query,
                    requestId,
                    correlationId);
                return true;
            }

            case PublicAccountCollectionStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (path == PublicChannelCollectionPath)
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path, requestId, correlationId);
            return true;
        }

        if (authorizedBackendIds.empty() ||
            authorizedBackendIds.size() > PublicChannelMaximumSources)
        {
            response = invalidRequestProblem(
                path,
                "One or more authorized backendId scopes are required.",
                requestId,
                correlationId);
            return true;
        }

        std::vector<std::string> normalizedBackends = authorizedBackendIds;
        std::sort(normalizedBackends.begin(), normalizedBackends.end());
        if (std::adjacent_find(
                normalizedBackends.begin(),
                normalizedBackends.end()) != normalizedBackends.end())
        {
            response = invalidRequestProblem(
                path,
                "Duplicate backendId scopes are not allowed.",
                requestId,
                correlationId);
            return true;
        }

        PublicChannelCollectionQuery query;
        if (!parsePublicChannelCollectionQuery(requestTarget, query) ||
            query.backendCount != normalizedBackends.size())
        {
            response = invalidRequestProblem(
                path,
                "The Channel collection query is invalid.",
                requestId,
                correlationId);
            return true;
        }

        const std::string backendScope =
            normalizedPublicChannelBackendScope(normalizedBackends);
        std::string afterBackendId;
        std::string afterChannelId;
        if (!query.cursor.empty())
        {
            const PublicChannelCursorDecodeStatus cursorStatus =
                decodePublicChannelCursor(
                    query.cursor,
                    backendScope,
                    afterBackendId,
                    afterChannelId);
            if (cursorStatus == PublicChannelCursorDecodeStatus::scopeMismatch)
            {
                response = cursorExpiredProblem(
                    path, requestId, correlationId);
                return true;
            }
            if (cursorStatus != PublicChannelCursorDecodeStatus::ok)
            {
                response = invalidRequestProblem(
                    path,
                    "The collection cursor is invalid for this ordering.",
                    requestId,
                    correlationId);
                return true;
            }
        }

        PublicChannelCollectionRequest request;
        request.backendIds = normalizedBackends;
        request.afterBackendId = afterBackendId;
        request.afterChannelId = afterChannelId;
        request.limit = query.limit;

        const PublicChannelCollectionResult page =
            lookupChannelCollection(request);

        switch (page.status)
        {
            case PublicChannelCollectionStatus::ok:
            {
                if (page.channels.size() > query.limit ||
                    (page.hasMore &&
                     page.channels.size() != query.limit) ||
                    page.sources.size() != normalizedBackends.size())
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }

                bool usefulSource = false;
                for (std::size_t index = 0U;
                     index < page.sources.size();
                     ++index)
                {
                    const PublicChannelCollectionSource& source =
                        page.sources[index];
                    const bool validState =
                        source.state == "ok" ||
                        source.state == "unavailable";
                    const bool validCode =
                        (source.state == "ok" && source.code.empty()) ||
                        (source.state == "unavailable" &&
                         source.code == "backend_unavailable");
                    if (source.backendId != normalizedBackends[index] ||
                        !validState ||
                        !validCode)
                    {
                        response = serviceUnavailableProblem(
                            path, requestId, correlationId);
                        return true;
                    }
                    usefulSource =
                        usefulSource || source.state == "ok";
                }
                if (!usefulSource)
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }

                std::string previousBackend = afterBackendId;
                std::string previousChannel = afterChannelId;
                for (const PublicChannelCollectionItem& channel :
                     page.channels)
                {
                    if (channel.backendId.empty() ||
                        channel.channelId.empty() ||
                        !std::binary_search(
                            normalizedBackends.begin(),
                            normalizedBackends.end(),
                            channel.backendId) ||
                        (!previousBackend.empty() &&
                         (channel.backendId < previousBackend ||
                          (channel.backendId == previousBackend &&
                           channel.channelId <= previousChannel))))
                    {
                        response = serviceUnavailableProblem(
                            path, requestId, correlationId);
                        return true;
                    }
                    previousBackend = channel.backendId;
                    previousChannel = channel.channelId;
                }

                response = publicChannelCollectionResponse(
                    page,
                    query,
                    normalizedBackends,
                    backendScope,
                    requestId,
                    correlationId);
                return true;
            }

            case PublicChannelCollectionStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The Channel collection request is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicChannelCollectionStatus::allSourcesUnavailable:
                response = backendUnavailableProblem(
                    path, requestId, correlationId);
                return true;

            case PublicChannelCollectionStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (path == PublicTimerAssignmentCollectionPath)
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path, requestId, correlationId);
            return true;
        }

        if (authorizedBackendId.empty())
        {
            response = invalidRequestProblem(
                path,
                "An authorized backend scope is required.",
                requestId,
                correlationId);
            return true;
        }

        PublicTimerAssignmentCollectionQuery query;
        if (!parsePublicTimerAssignmentCollectionQuery(
                requestTarget, query))
        {
            response = invalidRequestProblem(
                path,
                "The TimerAssignment collection query is invalid.",
                requestId,
                correlationId);
            return true;
        }

        std::string afterTimerAssignmentId;
        if (!query.cursor.empty() &&
            !decodePublicTimerAssignmentCursor(
                query.cursor,
                authorizedBackendId,
                afterTimerAssignmentId))
        {
            response = invalidRequestProblem(
                path,
                "The collection cursor is invalid for this backend or ordering.",
                requestId,
                correlationId);
            return true;
        }

        PublicTimerAssignmentCollectionRequest request;
        request.backendId = authorizedBackendId;
        request.afterTimerAssignmentId = afterTimerAssignmentId;
        request.limit = query.limit;

        const PublicTimerAssignmentCollectionResult page =
            lookupTimerAssignmentCollection(request);

        switch (page.status)
        {
            case PublicTimerAssignmentCollectionStatus::ok:
            {
                if (page.assignments.size() > query.limit ||
                    (page.hasMore &&
                     page.assignments.size() != query.limit))
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }

                std::string previous = afterTimerAssignmentId;
                for (const PublicTimerAssignmentCollectionItem&
                     assignment : page.assignments)
                {
                    if (assignment.timerAssignmentId.empty() ||
                        assignment.backendId != authorizedBackendId ||
                        (!previous.empty() &&
                         assignment.timerAssignmentId <= previous))
                    {
                        response = serviceUnavailableProblem(
                            path, requestId, correlationId);
                        return true;
                    }
                    previous = assignment.timerAssignmentId;
                }

                response =
                    publicTimerAssignmentCollectionResponse(
                        page,
                        query,
                        actorRef,
                        authorizedBackendId,
                        requestId,
                        correlationId);
                return true;
            }

            case PublicTimerAssignmentCollectionStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The TimerAssignment collection request is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicTimerAssignmentCollectionStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    std::string timerAssignmentId;
    if (publicTimerAssignmentPath(
            path,
            timerAssignmentId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        if (authorizedBackendId.empty())
        {
            response = invalidRequestProblem(
                path,
                "An authorized backend scope is required.",
                requestId,
                correlationId);
            return true;
        }

        const PublicTimerAssignmentLookupResult found =
            lookupTimerAssignment(
                timerAssignmentId,
                authorizedBackendId);

        switch (found.status)
        {
            case PublicTimerAssignmentLookupStatus::ok:
                if (found.assignment.timerAssignmentId !=
                        timerAssignmentId ||
                    found.assignment.backendId !=
                        authorizedBackendId ||
                    found.assignment.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                }
                else
                {
                    response =
                        publicTimerAssignmentResponse(
                            found.assignment,
                            path,
                            requestId,
                            correlationId,
                            ifNoneMatch);
                }
                return true;

            case PublicTimerAssignmentLookupStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The TimerAssignment identifier is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicTimerAssignmentLookupStatus::notFound:
                response = notFoundProblem(
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerAssignmentLookupStatus::unavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
    }

    if (isPublicV1Path(path))
    {
        response = notFoundProblem(
            path,
            requestId,
            correlationId);
        return true;
    }

    return false;
}

bool PublicApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& requestId,
    const std::string& correlationId,
    ApiResponse& response,
    const std::string& body,
    const std::string& actorRef,
    const std::string& ifMatch,
    const std::string& idempotencyKey,
    const std::string& contentType,
    const std::string& authorizedBackendId) const
{
    const std::string path = requestPath(requestTarget);
    std::string operationId;
    std::string timerAssignmentId;
    std::string accountId;

    if (path == PublicAccountCollectionPath)
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path, requestId, correlationId);
            return true;
        }

        if (requestTarget != path)
        {
            response = invalidRequestProblem(
                path,
                "Account CREATE does not accept query parameters.",
                requestId,
                correlationId);
            return true;
        }

        if (!applicationJsonContentType(contentType))
        {
            response = problemResponse(
                415,
                "invalid_request",
                "Unsupported media type",
                "Account CREATE requires Content-Type application/json.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (body.size() > 4096U)
        {
            response = invalidRequestProblem(
                path,
                "The Account CREATE request body is too large.",
                requestId,
                correlationId);
            return true;
        }

        JsonSyntaxValidator validator(body);
        if (!validator.valid())
        {
            response = problemResponse(
                400,
                "invalid_request",
                "Invalid JSON",
                "The Account CREATE request body is not valid JSON.",
                path,
                requestId,
                correlationId);
            return true;
        }

        std::string loginName;
        std::string displayName;
        std::string password;
        if (!parsePublicAccountCreateBody(
                body,
                loginName,
                displayName,
                password))
        {
            response = problemResponse(
                422,
                "validation_error",
                "Validation failed",
                "Account CREATE requires exactly loginName, displayName and password string fields.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (!publicIdempotencyKeyValid(idempotencyKey))
        {
            response = invalidRequestProblem(
                path,
                idempotencyKey.empty()
                    ? "Idempotency-Key is required for Account CREATE."
                    : "Idempotency-Key is malformed or too long.",
                requestId,
                correlationId);
            return true;
        }

        AccountCreate create;
        {
            std::lock_guard<std::mutex> lock(
                accountCreateMutex_);
            create = accountCreate_;
        }

        if (!create)
        {
            response = serviceUnavailableProblem(
                path, requestId, correlationId);
            return true;
        }

        PublicAccountCreateRequest createRequest;
        createRequest.actorRef = actorRef;
        createRequest.loginName = std::move(loginName);
        createRequest.displayName = std::move(displayName);
        createRequest.password = std::move(password);
        createRequest.idempotencyKey = idempotencyKey;
        createRequest.requestId = requestId;
        createRequest.correlationId = correlationId;

        const PublicAccountCreateResult created =
            create(createRequest);
        std::fill(
            createRequest.password.begin(),
            createRequest.password.end(),
            '\0');
        createRequest.password.clear();

        switch (created.status)
        {
            case PublicAccountCreateStatus::created:
            case PublicAccountCreateStatus::replayed:
            {
                if (created.account.accountId.empty() ||
                    created.account.actorId.empty() ||
                    created.account.displayName.empty() ||
                    created.account.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }

                const std::string accountPath =
                    std::string(PublicAccountPrefix) +
                    created.account.accountId;
                response = publicAccountResponse(
                    created.account,
                    accountPath,
                    requestId,
                    correlationId,
                    "");
                response.statusCode = 201;
                response.headers["Location"] = accountPath;
                return true;
            }

            case PublicAccountCreateStatus::invalid:
                response = problemResponse(
                    422,
                    "validation_error",
                    "Validation failed",
                    "The Account CREATE submission is not valid.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountCreateStatus::loginConflict:
                response = problemResponse(
                    409,
                    "operation_conflict",
                    "Operation conflict",
                    "The requested loginName is already in use.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountCreateStatus::idempotencyConflict:
                response = problemResponse(
                    409,
                    "idempotency_conflict",
                    "Idempotency conflict",
                    "Idempotency-Key was already used for different Account CREATE fields.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountCreateStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (publicAccountPath(
            path,
            accountId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        if (!applicationJsonContentType(contentType))
        {
            response = problemResponse(
                415,
                "invalid_request",
                "Unsupported media type",
                "Account mutation requires Content-Type application/json.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (body.size() > 4096U)
        {
            response = invalidRequestProblem(
                path,
                "The Account mutation request body is too large.",
                requestId,
                correlationId);
            return true;
        }

        JsonSyntaxValidator validator(body);
        if (!validator.valid())
        {
            response = problemResponse(
                400,
                "invalid_request",
                "Invalid JSON",
                "The Account mutation request body is not valid JSON.",
                path,
                requestId,
                correlationId);
            return true;
        }

        PublicAccountMutationKind mutationKind =
            PublicAccountMutationKind::displayName;
        std::string displayName;
        bool active = false;
        if (!parsePublicAccountMutationBody(
                body,
                mutationKind,
                displayName,
                active))
        {
            response = problemResponse(
                422,
                "validation_error",
                "Validation failed",
                "Account mutation accepts exactly one field: displayName or active.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (ifMatch.empty())
        {
            response = problemResponse(
                428,
                "precondition_required",
                "Precondition required",
                "Account mutation requires one strong If-Match entity tag.",
                path,
                requestId,
                correlationId);
            return true;
        }

        std::string expectedResourceRevision;
        if (!vdrsuite::http::publicStrongEntityTagResourceRevision(
                ifMatch,
                expectedResourceRevision))
        {
            response = invalidRequestProblem(
                path,
                "If-Match must contain exactly one canonical strong VDR-Suite entity tag.",
                requestId,
                correlationId);
            return true;
        }

        std::uint64_t expectedRevision = 0U;
        if (!publicAccountRevision(
                expectedResourceRevision,
                expectedRevision))
        {
            response = invalidRequestProblem(
                path,
                "If-Match does not identify an Account revision.",
                requestId,
                correlationId);
            return true;
        }

        AccountMutation mutation;
        {
            std::lock_guard<std::mutex> lock(
                accountMutationMutex_);
            mutation = accountMutation_;
        }

        if (!mutation)
        {
            response = serviceUnavailableProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        PublicAccountMutationRequest mutationRequest;
        mutationRequest.actorRef = actorRef;
        mutationRequest.accountId = accountId;
        mutationRequest.expectedRevision = expectedRevision;
        mutationRequest.kind = mutationKind;
        mutationRequest.displayName = displayName;
        mutationRequest.active = active;
        mutationRequest.requestId = requestId;
        mutationRequest.correlationId = correlationId;

        const PublicAccountMutationResult mutated =
            mutation(mutationRequest);

        switch (mutated.status)
        {
            case PublicAccountMutationStatus::ok:
                if (mutated.account.accountId != accountId ||
                    mutated.account.actorId.empty() ||
                    mutated.account.displayName.empty() ||
                    mutated.account.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                }
                else
                {
                    response = publicAccountResponse(
                        mutated.account,
                        path,
                        requestId,
                        correlationId,
                        "");
                }
                return true;

            case PublicAccountMutationStatus::invalid:
                response = problemResponse(
                    422,
                    "validation_error",
                    "Validation failed",
                    "The Account mutation is not valid.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountMutationStatus::notFound:
                response = notFoundProblem(
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountMutationStatus::revisionConflict:
                response = problemResponse(
                    412,
                    "revision_conflict",
                    "Resource revision conflict",
                    "The Account changed after it was read.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountMutationStatus::finalAdministrator:
                response = problemResponse(
                    409,
                    "operation_conflict",
                    "Operation conflict",
                    "The final usable administrator cannot be deactivated.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountMutationStatus::unavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
    }

    if (publicTimerAssignmentPath(
            path,
            timerAssignmentId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        if (authorizedBackendId.empty())
        {
            response = invalidRequestProblem(
                path,
                "An authorized backend scope is required.",
                requestId,
                correlationId);
            return true;
        }

        if (!applicationJsonContentType(contentType))
        {
            response = publicTimerCreateProblem(
                415,
                "unsupported_media_type",
                "Unsupported media type",
                "Timer CREATE requires Content-Type application/json.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (body.size() > 4096U)
        {
            response = invalidRequestProblem(
                path,
                "The Timer CREATE request body is too large.",
                requestId,
                correlationId);
            return true;
        }

        JsonSyntaxValidator validator(body);
        if (!validator.valid())
        {
            response = publicTimerCreateProblem(
                400,
                "invalid_json",
                "Invalid JSON",
                "The Timer CREATE request body is not valid JSON.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (!emptyJsonObject(body))
        {
            response = publicTimerCreateProblem(
                422,
                "validation_error",
                "Validation failed",
                "Timer CREATE accepts a closed empty JSON object.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (ifMatch.empty())
        {
            response = publicTimerCreateProblem(
                428,
                "precondition_required",
                "Precondition required",
                "Timer CREATE requires one strong If-Match entity tag.",
                path,
                requestId,
                correlationId);
            return true;
        }

        std::string expectedAssignmentRevision;
        if (!vdrsuite::http::publicStrongEntityTagResourceRevision(
                ifMatch,
                expectedAssignmentRevision))
        {
            response = invalidRequestProblem(
                path,
                "If-Match must contain exactly one canonical strong VDR-Suite entity tag.",
                requestId,
                correlationId);
            return true;
        }

        if (!publicIdempotencyKeyValid(idempotencyKey))
        {
            response = invalidRequestProblem(
                path,
                idempotencyKey.empty()
                    ? "Idempotency-Key is required for Timer CREATE."
                    : "Idempotency-Key is malformed or too long.",
                requestId,
                correlationId);
            return true;
        }

        TimerCreateAdmission admission;
        {
            std::lock_guard<std::mutex> lock(
                timerCreateAdmissionMutex_);
            admission = timerCreateAdmission_;
        }

        if (!admission)
        {
            response = serviceUnavailableProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        PublicTimerCreateAdmissionRequest admissionRequest;
        admissionRequest.actorRef = actorRef;
        admissionRequest.backendId = authorizedBackendId;
        admissionRequest.timerAssignmentId = timerAssignmentId;
        admissionRequest.expectedAssignmentRevision =
            expectedAssignmentRevision;
        admissionRequest.idempotencyKey = idempotencyKey;

        const PublicTimerCreateAdmissionResult admitted =
            admission(admissionRequest);

        switch (admitted.status)
        {
            case PublicTimerCreateAdmissionStatus::accepted:
            case PublicTimerCreateAdmissionStatus::replayed:
            {
                if (admitted.operation.operationId.empty() ||
                    admitted.operation.state.empty() ||
                    admitted.operation.backendId !=
                        authorizedBackendId ||
                    admitted.operation.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                    return true;
                }

                const std::string operationPath =
                    std::string(PublicOperationPrefix) +
                    admitted.operation.operationId;
                response = publicOperationResponse(
                    admitted.operation,
                    operationPath,
                    requestId,
                    correlationId,
                    "");
                response.statusCode = 202;
                response.headers["Location"] = operationPath;
                return true;
            }

            case PublicTimerCreateAdmissionStatus::invalid:
                response = publicTimerCreateProblem(
                    422,
                    "validation_error",
                    "Validation failed",
                    "The Timer CREATE submission is not valid.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::notFound:
                response = notFoundProblem(
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::readOnlyBackend:
                response = publicTimerCreateProblem(
                    403,
                    "read_only_backend",
                    "Backend is read-only",
                    "The selected backend does not permit Timer mutation.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::backendUnavailable:
                response = publicTimerCreateProblem(
                    503,
                    "backend_unavailable",
                    "Backend unavailable",
                    "The selected backend cannot currently accept Timer mutation.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::revisionConflict:
                response = publicTimerCreateProblem(
                    412,
                    "revision_conflict",
                    "Resource revision conflict",
                    "The TimerAssignment changed after it was read.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::stateConflict:
                response = publicTimerCreateProblem(
                    409,
                    "operation_conflict",
                    "Operation conflict",
                    "The TimerAssignment state does not permit Timer CREATE.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::generationConflict:
                response = publicTimerCreateProblem(
                    409,
                    "generation_conflict",
                    "Backend generation conflict",
                    "The TimerAssignment backend generation is no longer current.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::idempotencyConflict:
                response = publicTimerCreateProblem(
                    409,
                    "idempotency_conflict",
                    "Idempotency conflict",
                    "Idempotency-Key was already used for a different Timer CREATE submission.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::operationConflict:
                response = publicTimerCreateProblem(
                    409,
                    "operation_conflict",
                    "Operation conflict",
                    "The durable operation state conflicts with Timer CREATE.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::serviceUnavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
    }

    if (path == PublicAccountCollectionPath)
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId,
            "GET, POST");
        return true;
    }

    if (path == "/api/v1" ||
        path == "/api/v1/capabilities" ||
        path == PublicBackendCollectionPath ||
        path == PublicChannelCollectionPath ||
        path == PublicTimerAssignmentCollectionPath ||
        publicOperationPath(path, operationId))
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId);
        return true;
    }

    return false;
}

bool PublicApiRuntime::tryHandleUnsupportedMethod(
    const std::string& method,
    const std::string& requestTarget,
    const std::string& requestId,
    const std::string& correlationId,
    ApiResponse& response) const
{
    if (method == "GET" ||
        method == "POST")
    {
        return false;
    }

    const std::string path = requestPath(requestTarget);
    std::string operationId;
    std::string timerAssignmentId;
    std::string accountId;

    if (publicTimerAssignmentPath(
            path,
            timerAssignmentId))
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId,
            "GET, POST");
        return true;
    }

    if (publicAccountPath(path, accountId))
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId,
            "GET, POST");
        return true;
    }

    if (path == PublicAccountCollectionPath)
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId,
            "GET, POST");
        return true;
    }

    if (path == "/api/v1" ||
        path == "/api/v1/capabilities" ||
        path == PublicBackendCollectionPath ||
        path == PublicChannelCollectionPath ||
        path == PublicTimerAssignmentCollectionPath ||
        publicOperationPath(path, operationId))
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId);
        return true;
    }

    if (isPublicV1Path(path))
    {
        response = notFoundProblem(
            path,
            requestId,
            correlationId);
        return true;
    }

    return false;
}
