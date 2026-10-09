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
constexpr const char* PublicDevicePairingCollectionPath =
    "/api/v1/device-pairings";
constexpr const char* PublicDevicePairingPrefix =
    "/api/v1/device-pairings/";
constexpr std::size_t PublicDevicePairingDefaultLimit = 50U;
constexpr std::size_t PublicDevicePairingMaximumLimit = 100U;
constexpr const char* PublicDevicePairingCollectionSort =
    "pairingRequestId";
constexpr const char* PublicDevicePairingCollectionOrder = "asc";
constexpr const char* PublicDevicePairingCursorPrefix = "dp1_";
constexpr const char* PublicDevicePairingCursorPayloadVersion =
    "device-pairings/1|";
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
constexpr const char* PublicRecordingCollectionPath = "/api/v1/recordings";
constexpr std::size_t PublicRecordingMaximumLimit = 100U;
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

bool publicDevicePairingPath(
    const std::string& path,
    std::string& pairingRequestId)
{
    const std::string prefix(PublicDevicePairingPrefix);
    if (path.compare(0U, prefix.size(), prefix) != 0)
        return false;

    pairingRequestId = path.substr(prefix.size());
    return !pairingRequestId.empty() &&
        pairingRequestId.find('/') == std::string::npos;
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

bool publicAccountSubresourcePath(
    const std::string& path,
    const std::string& suffix,
    std::string& accountId)
{
    const std::string prefix(PublicAccountPrefix);
    if (path.compare(0, prefix.size(), prefix) != 0 ||
        path.size() <= prefix.size() + suffix.size() ||
        path.compare(
            path.size() - suffix.size(),
            suffix.size(),
            suffix) != 0)
    {
        return false;
    }

    accountId = path.substr(
        prefix.size(),
        path.size() - prefix.size() - suffix.size());
    return !accountId.empty() &&
        accountId.find('/') == std::string::npos;
}

bool publicDeviceLifecyclePath(
    const std::string& path, std::string& deviceId,
    std::string& credentialId)
{
    static const std::string prefix = "/api/v1/devices/";
    static const std::string suffix = "/lifecycle";
    static const std::string marker = "/credentials/";
    deviceId.clear();
    credentialId.clear();
    if (path.compare(0U, prefix.size(), prefix) != 0 ||
        path.size() <= prefix.size() + suffix.size() ||
        path.compare(path.size() - suffix.size(),
                     suffix.size(), suffix) != 0)
        return false;
    const std::string middle = path.substr(
        prefix.size(), path.size() - prefix.size() - suffix.size());
    const std::size_t position = middle.find(marker);
    if (position == std::string::npos)
    {
        if (middle.empty() || middle.find('/') != std::string::npos)
            return false;
        deviceId = middle;
        return true;
    }
    deviceId = middle.substr(0U, position);
    credentialId = middle.substr(position + marker.size());
    return !deviceId.empty() && !credentialId.empty() &&
        deviceId.find('/') == std::string::npos &&
        credentialId.find('/') == std::string::npos;
}

bool publicDeviceCredentialRotationPath(
    const std::string& path, std::string& deviceId,
    std::string& credentialId)
{
    static const std::string prefix = "/api/v1/devices/";
    static const std::string marker = "/credentials/";
    static const std::string suffix = "/rotate";
    deviceId.clear();
    credentialId.clear();
    if (path.compare(0U, prefix.size(), prefix) != 0 ||
        path.size() <= prefix.size() + marker.size() + suffix.size() ||
        path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
        return false;
    const std::string middle = path.substr(
        prefix.size(), path.size() - prefix.size() - suffix.size());
    const auto i = middle.find(marker);
    if (i == std::string::npos) return false;
    deviceId = middle.substr(0U, i);
    credentialId = middle.substr(i + marker.size());
    return !deviceId.empty() && !credentialId.empty() &&
        deviceId.find('/') == std::string::npos &&
        credentialId.find('/') == std::string::npos;
}

bool publicDeviceGrantPath(
    const std::string& path, std::string& deviceId)
{
    static const std::string prefix = "/api/v1/devices/";
    static const std::string suffix = "/grants";
    if (path.compare(0U, prefix.size(), prefix) != 0 ||
        path.size() <= prefix.size() + suffix.size() ||
        path.compare(path.size() - suffix.size(),
                     suffix.size(), suffix) != 0)
        return false;
    deviceId = path.substr(
        prefix.size(), path.size() - prefix.size() - suffix.size());
    return !deviceId.empty() &&
        deviceId.find('/') == std::string::npos;
}

bool publicAccountGrantPath(
    const std::string& path,
    std::string& accountId)
{
    return publicAccountSubresourcePath(
        path,
        "/grants",
        accountId);
}

bool publicAccountCredentialPath(
    const std::string& path,
    std::string& accountId)
{
    return publicAccountSubresourcePath(
        path,
        "/credentials",
        accountId);
}

bool publicAccountSessionPath(
    const std::string& path,
    std::string& accountId)
{
    return publicAccountSubresourcePath(
        path,
        "/sessions",
        accountId);
}

bool publicAccountCredentialItemPath(
    const std::string& path,
    std::string& accountId,
    std::string& credentialId)
{
    static const std::string Prefix = "/api/v1/accounts/";
    static const std::string Marker = "/credentials/";

    if (path.compare(0U, Prefix.size(), Prefix) != 0)
        return false;

    const std::size_t marker =
        path.find(Marker, Prefix.size());
    if (marker == std::string::npos)
        return false;

    accountId = path.substr(
        Prefix.size(),
        marker - Prefix.size());
    credentialId = path.substr(marker + Marker.size());

    return !accountId.empty() &&
        accountId.find('/') == std::string::npos &&
        !credentialId.empty() &&
        credentialId.find('/') == std::string::npos;
}

bool publicAccountSessionItemPath(
    const std::string& path,
    std::string& accountId,
    std::string& sessionId)
{
    static const std::string Prefix = "/api/v1/accounts/";
    static const std::string Marker = "/sessions/";

    if (path.compare(0U, Prefix.size(), Prefix) != 0)
        return false;

    const std::size_t marker =
        path.find(Marker, Prefix.size());
    if (marker == std::string::npos)
        return false;

    accountId = path.substr(
        Prefix.size(),
        marker - Prefix.size());
    sessionId = path.substr(marker + Marker.size());

    return !accountId.empty() &&
        accountId.find('/') == std::string::npos &&
        !sessionId.empty() &&
        sessionId.find('/') == std::string::npos;
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

struct PublicDevicePairingCollectionQuery
{
    std::size_t limit = PublicDevicePairingDefaultLimit;
    std::string cursor;
};

bool parsePublicDevicePairingCollectionQuery(
    const std::string& requestTarget,
    PublicDevicePairingCollectionQuery& query)
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
        const std::size_t separator =
            encoded.find('&', position);
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
                parsed > PublicDevicePairingMaximumLimit)
            {
                return false;
            }
            query.limit = parsed;
        }
        else if (key == "cursor")
        {
            if (cursorSeen ||
                value.empty() ||
                value.size() > 4096U)
            {
                return false;
            }
            cursorSeen = true;
            query.cursor = value;
        }
        else if (key == "sort")
        {
            if (sortSeen ||
                value != PublicDevicePairingCollectionSort)
            {
                return false;
            }
            sortSeen = true;
        }
        else if (key == "order")
        {
            if (orderSeen ||
                value != PublicDevicePairingCollectionOrder)
            {
                return false;
            }
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

std::string publicDevicePairingCursor(
    const std::string& lastPairingRequestId)
{
    if (lastPairingRequestId.empty()) return "";

    std::string payload(
        PublicDevicePairingCursorPayloadVersion);
    appendCursorField(payload, lastPairingRequestId);
    return std::string(PublicDevicePairingCursorPrefix) +
        hexEncode(payload);
}

bool decodePublicDevicePairingCursor(
    const std::string& cursor,
    std::string& lastPairingRequestId)
{
    const std::string prefix(
        PublicDevicePairingCursorPrefix);
    if (cursor.size() <= prefix.size() ||
        cursor.compare(0U, prefix.size(), prefix) != 0)
    {
        return false;
    }

    std::string payload;
    if (!hexDecode(cursor.substr(prefix.size()), payload))
        return false;

    const std::string version(
        PublicDevicePairingCursorPayloadVersion);
    if (payload.compare(0U, version.size(), version) != 0)
        return false;

    std::size_t position = version.size();
    if (!readCursorField(
            payload,
            position,
            lastPairingRequestId) ||
        position != payload.size() ||
        lastPairingRequestId.empty() ||
        lastPairingRequestId.size() > 128U)
    {
        return false;
    }

    return true;
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

std::string publicDevicePairingCollectionTarget(
    std::size_t limit,
    const std::string& cursor)
{
    std::string target =
        std::string(PublicDevicePairingCollectionPath) +
        "?limit=" + std::to_string(limit) +
        "&sort=" + PublicDevicePairingCollectionSort +
        "&order=" + PublicDevicePairingCollectionOrder;
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

bool parsePublicDevicePairingCreateBody(
    const std::string& input,
    std::string& displayName,
    std::string& clientKind,
    std::string& appVersion)
{
    std::size_t position = 0U;
    bool displaySeen = false;
    bool kindSeen = false;
    bool versionSeen = false;

    skipAccountMutationWhitespace(input, position);
    if (position >= input.size() ||
        input[position++] != '{')
    {
        return false;
    }

    while (true)
    {
        skipAccountMutationWhitespace(input, position);
        if (position < input.size() &&
            input[position] == '}')
        {
            ++position;
            break;
        }

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

        std::string value;
        if (!parseAccountMutationJsonString(
                input,
                position,
                value))
        {
            return false;
        }

        if (key == "displayName" && !displaySeen)
        {
            displaySeen = true;
            displayName = std::move(value);
        }
        else if (key == "clientKind" && !kindSeen)
        {
            kindSeen = true;
            clientKind = std::move(value);
        }
        else if (key == "appVersion" && !versionSeen)
        {
            versionSeen = true;
            appVersion = std::move(value);
        }
        else
        {
            return false;
        }

        skipAccountMutationWhitespace(input, position);
        if (position >= input.size())
            return false;
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
        displaySeen &&
        kindSeen &&
        !displayName.empty() &&
        !clientKind.empty();
}

bool parsePublicDevicePairingDecisionBody(
    const std::string& input,
    std::string& decision)
{
    std::size_t position = 0U;
    bool decisionSeen = false;

    skipAccountMutationWhitespace(input, position);
    if (position >= input.size() ||
        input[position++] != '{')
    {
        return false;
    }

    while (true)
    {
        skipAccountMutationWhitespace(input, position);
        if (position < input.size() &&
            input[position] == '}')
        {
            ++position;
            break;
        }

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
        std::string value;
        if (!parseAccountMutationJsonString(
                input,
                position,
                value))
        {
            return false;
        }

        if (key == "decision" && !decisionSeen)
        {
            decisionSeen = true;
            decision = std::move(value);
        }
        else
        {
            return false;
        }

        skipAccountMutationWhitespace(input, position);
        if (position >= input.size())
            return false;
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
        decisionSeen &&
        (decision == "approve" ||
         decision == "reject");
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

bool publicDevicePairingRevision(
    const std::string& resourceRevision,
    const std::string& pairingRequestId)
{
    const std::string prefix =
        "device-pairing:" + pairingRequestId + ":";
    if (pairingRequestId.empty() ||
        resourceRevision.size() <= prefix.size() ||
        resourceRevision.compare(
            0U,
            prefix.size(),
            prefix) != 0)
    {
        return false;
    }

    std::uint64_t parsed = 0U;
    for (std::size_t index = prefix.size();
         index < resourceRevision.size();
         ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(
                resourceRevision[index]);
        if (character < '0' || character > '9')
            return false;

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

    return parsed > 0U;
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

bool publicCredentialLifecycleRevision(
    const std::string& resourceRevision)
{
    static const std::string Prefix = "credential-lifecycle:";
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

bool publicSessionLifecycleRevision(
    const std::string& resourceRevision)
{
    static const std::string Prefix = "session-lifecycle:";
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
        + "},\"links\":{\"self\":\"/api/v1\",\"capabilities\":\"/api/v1/capabilities\",\"backends\":\"/api/v1/backends\",\"accounts\":\"/api/v1/accounts\",\"devicePairings\":\"/api/v1/device-pairings\"}}",
        requestId,
        correlationId);
}

std::string publicDevicePairingJson(
    const PublicDevicePairingResource& resource,
    const std::string& self)
{
    std::string body =
        "{\"pairingRequestId\":\"" +
        jsonEscape(resource.pairingRequestId) +
        "\",\"status\":\"" +
        jsonEscape(resource.state) +
        "\",\"expiresAt\":\"" +
        jsonEscape(resource.expiresAt) +
        "\",\"pollIntervalSeconds\":" +
        std::to_string(resource.pollIntervalSeconds) +
        ",\"client\":{\"displayName\":\"" +
        jsonEscape(resource.client.displayName) +
        "\",\"clientKind\":\"" +
        jsonEscape(resource.client.clientKind) +
        "\",\"appVersion\":\"" +
        jsonEscape(resource.client.appVersion) +
        "\"},\"links\":{\"self\":\"" +
        jsonEscape(self) +
        "\"}}";
    return body;
}

ApiResponse publicDevicePairingResponse(
    const PublicDevicePairingResource& resource,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return jsonResponse(
        publicDevicePairingJson(resource, path),
        requestId,
        correlationId);
}

std::string publicDevicePairingAdministrativeJson(
    const PublicDevicePairingAdministrativeResource& resource,
    const std::string& self)
{
    std::string body =
        "{\"pairingRequestId\":\"" +
        jsonEscape(resource.resource.pairingRequestId) +
        "\",\"status\":\"" +
        jsonEscape(resource.resource.state) +
        "\",\"expiresAt\":\"" +
        jsonEscape(resource.resource.expiresAt) +
        "\",\"pollIntervalSeconds\":" +
        std::to_string(
            resource.resource.pollIntervalSeconds) +
        ",\"client\":{\"displayName\":\"" +
        jsonEscape(resource.resource.client.displayName) +
        "\",\"clientKind\":\"" +
        jsonEscape(resource.resource.client.clientKind) +
        "\",\"appVersion\":\"" +
        jsonEscape(resource.resource.client.appVersion) +
        "\"},\"decision\":{\"decidedByActorId\":";

    body += resource.decidedByActorId.empty()
        ? "null"
        : "\"" +
            jsonEscape(resource.decidedByActorId) +
            "\"";
    body += ",\"decidedAt\":";
    body += resource.decidedAt.empty()
        ? "null"
        : "\"" + jsonEscape(resource.decidedAt) + "\"";
    body +=
        "},\"links\":{\"self\":\"" +
        jsonEscape(self) +
        "\"}}";
    return body;
}

ApiResponse publicDevicePairingAdministrationResponse(
    const PublicDevicePairingAdministrativeResource& resource,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    const std::string entityTag =
        vdrsuite::http::publicStrongEntityTag(
            resource.resourceRevision);
    if (entityTag.empty())
    {
        return serviceUnavailableProblem(
            path,
            requestId,
            correlationId);
    }

    ApiResponse response =
        jsonResponse(
            publicDevicePairingAdministrativeJson(
                resource,
                path),
            requestId,
            correlationId);
    response.headers["ETag"] = entityTag;
    return response;
}

ApiResponse publicDevicePairingAdministrationCollectionResponse(
    const PublicDevicePairingAdministrationCollectionResult& page,
    const PublicDevicePairingCollectionQuery& query,
    const std::string& requestId,
    const std::string& correlationId)
{
    const std::string nextCursor =
        page.hasMore && !page.requests.empty()
            ? publicDevicePairingCursor(
                page.requests.back()
                    .resource.pairingRequestId)
            : std::string();

    if (page.hasMore && nextCursor.empty())
    {
        return serviceUnavailableProblem(
            PublicDevicePairingCollectionPath,
            requestId,
            correlationId);
    }

    const std::string self =
        publicDevicePairingCollectionTarget(
            query.limit,
            query.cursor);
    const std::string next =
        nextCursor.empty()
            ? std::string()
            : publicDevicePairingCollectionTarget(
                query.limit,
                nextCursor);

    std::string body = "{\"items\":[";
    for (std::size_t index = 0U;
         index < page.requests.size();
         ++index)
    {
        if (index > 0U) body += ",";
        const auto& item = page.requests[index];
        body += publicDevicePairingAdministrativeJson(
            item,
            std::string(PublicDevicePairingPrefix) +
                item.resource.pairingRequestId);
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
        jsonEscape(self) +
        "\",\"next\":";
    body += next.empty()
        ? "null"
        : "\"" + jsonEscape(next) + "\"";
    body += "}}";

    return jsonResponse(
        body,
        requestId,
        correlationId);
}

ApiResponse publicDevicePairingCreatedResponse(
    const PublicDevicePairingCreateResult& created,
    const std::string& requestId,
    const std::string& correlationId)
{
    const std::string self =
        std::string(PublicDevicePairingPrefix) +
        created.resource.pairingRequestId;
    std::string body =
        "{\"pairingRequestId\":\"" +
        jsonEscape(created.resource.pairingRequestId) +
        "\",\"userCode\":\"" +
        jsonEscape(created.userCode) +
        "\",\"pairingToken\":\"" +
        jsonEscape(created.pairingToken) +
        "\",\"status\":\"" +
        jsonEscape(created.resource.state) +
        "\",\"expiresAt\":\"" +
        jsonEscape(created.resource.expiresAt) +
        "\",\"pollIntervalSeconds\":" +
        std::to_string(
            created.resource.pollIntervalSeconds) +
        ",\"client\":{\"displayName\":\"" +
        jsonEscape(created.resource.client.displayName) +
        "\",\"clientKind\":\"" +
        jsonEscape(created.resource.client.clientKind) +
        "\",\"appVersion\":\"" +
        jsonEscape(created.resource.client.appVersion) +
        "\"},\"links\":{\"self\":\"" +
        jsonEscape(self) +
        "\"}}";

    ApiResponse response =
        jsonResponse(
            body,
            requestId,
            correlationId);
    response.statusCode = 201;
    response.headers["Location"] = self;
    return response;
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
    const bool deviceGrantAdministrationAvailable,
    const bool deviceLifecycleAdministrationAvailable,
    const bool deviceCredentialRotationAvailable,
    const bool accountSecurityMetadataAvailable,
    const bool accountCredentialRevokeAvailable,
    const bool accountSessionRevokeAvailable,
    const bool devicePairingBootstrapAvailable,
    const bool devicePairingAdministrationAvailable,
    const bool deviceCredentialIssueAvailable,
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
        "{\"id\":\"public-api.devices-grants-administration\",\"version\":1,\"availability\":\"" +
        std::string(deviceGrantAdministrationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.devices-lifecycle-administration\",\"version\":1,\"availability\":\"" +
        std::string(deviceLifecycleAdministrationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.devices-credential-rotation\",\"version\":1,\"availability\":\"" +
        std::string(deviceCredentialRotationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-credential-session-metadata\",\"version\":1,\"availability\":\"" +
        std::string(accountSecurityMetadataAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-credential-revoke\",\"version\":1,\"availability\":\"" +
        std::string(accountCredentialRevokeAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-session-revoke\",\"version\":1,\"availability\":\"" +
        std::string(accountSessionRevokeAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.device-pairing-bootstrap\",\"version\":1,\"availability\":\"" +
        std::string(devicePairingBootstrapAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.device-pairing-administration\",\"version\":1,\"availability\":\"" +
        std::string(devicePairingAdministrationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.device-credential-issuance\",\"version\":1,\"availability\":\"" +
        std::string(deviceCredentialIssueAvailable ? "available" : "unavailable") +
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

ApiResponse publicDeviceLifecycleResponse(
    const PublicDeviceLifecycleResource& item,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& ifNoneMatch)
{
    const std::string etag =
        vdrsuite::http::publicStrongEntityTag(item.resourceRevision);
    if (etag.empty())
        return serviceUnavailableProblem(path, requestId, correlationId);
    const auto condition = vdrsuite::http::publicEvaluateIfNoneMatch(
        ifNoneMatch, etag);
    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::malformed)
        return invalidRequestProblem(
            path, "Invalid If-None-Match.", requestId, correlationId);
    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::matched)
    {
        ApiResponse response;
        response.statusCode = 304;
        response.contentType = "application/json; charset=utf-8";
        addPublicSuccessHeaders(response, requestId, correlationId);
        response.headers["ETag"] = etag;
        response.headers["Cache-Control"] = "no-store";
        return response;
    }
    std::string body = "{\"deviceId\":\"" +
        jsonEscape(item.deviceId) + "\",\"actorId\":\"" +
        jsonEscape(item.actorId) + "\",\"credentialId\":\"" +
        jsonEscape(item.credentialId) + "\",\"active\":" +
        std::string(item.active ? "true" : "false") +
        ",\"revoked\":" + std::string(item.revoked ? "true" : "false") +
        "}";
    ApiResponse response = jsonResponse(body, requestId, correlationId);
    response.headers["ETag"] = etag;
    response.headers["Cache-Control"] = "no-store";
    return response;
}

ApiResponse publicDeviceGrantSetResponse(
    const PublicDeviceGrantSetResource& grantSet,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& ifNoneMatch)
{
    const std::string etag = vdrsuite::http::publicStrongEntityTag(
        grantSet.resourceRevision);
    if (etag.empty())
        return serviceUnavailableProblem(path, requestId, correlationId);

    const auto condition = vdrsuite::http::publicEvaluateIfNoneMatch(
        ifNoneMatch, etag);
    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::malformed)
        return invalidRequestProblem(
            path, "Malformed If-None-Match.", requestId, correlationId);
    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::matched)
    {
        ApiResponse result;
        result.statusCode = 304;
        result.contentType = "application/json; charset=utf-8";
        addPublicSuccessHeaders(result, requestId, correlationId);
        result.headers["ETag"] = etag;
        return result;
    }

    std::string body = "{\"deviceId\":\"" +
        jsonEscape(grantSet.deviceId) +
        "\",\"actorId\":\"" +
        jsonEscape(grantSet.actorId) + "\",\"items\":[";
    for (std::size_t i = 0; i < grantSet.grants.size(); ++i)
    {
        if (i != 0U) body += ",";
        body += "{\"permission\":\"" +
            jsonEscape(grantSet.grants[i].permission) +
            "\",\"backendId\":\"" +
            jsonEscape(grantSet.grants[i].backendId) + "\"}";
    }
    body += "],\"supportedPermissions\":["
        "\"channels.view\",\"timers.view\","
        "\"media.live.play\",\"media.recording.play\"]}";
    ApiResponse result = jsonResponse(body, requestId, correlationId);
    result.headers["ETag"] = etag;
    result.headers["Cache-Control"] = "no-store";
    return result;
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
    body += "],\"supportedPermissions\":[";
    for (std::size_t index = 0U;
         index < grantSet.supportedPermissions.size();
         ++index)
    {
        if (index > 0U) body += ",";
        body += "\"" +
            jsonEscape(grantSet.supportedPermissions[index]) +
            "\"";
    }
    body += "],\"supportedPermissionOptions\":[";
    for (std::size_t index = 0U;
         index < grantSet.supportedPermissionOptions.size();
         ++index)
    {
        if (index > 0U) body += ",";
        const PublicAccountGrantOption& option =
            grantSet.supportedPermissionOptions[index];
        body +=
            "{\"permission\":\"" +
            jsonEscape(option.permission) +
            "\",\"presentationKey\":\"" +
            jsonEscape(option.presentationKey) +
            "\",\"category\":\"" +
            jsonEscape(option.category) +
            "\"}";
    }
    body += "],\"supportedScopeKinds\":[";
    for (std::size_t index = 0U;
         index < grantSet.supportedScopeKinds.size();
         ++index)
    {
        if (index > 0U) body += ",";
        body += "\"" +
            jsonEscape(grantSet.supportedScopeKinds[index]) +
            "\"";
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

ApiResponse publicAccountCredentialResourceResponse(
    const PublicAccountCredentialResource& resource,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& ifNoneMatch)
{
    if (!publicCredentialLifecycleRevision(
            resource.resourceRevision))
    {
        return serviceUnavailableProblem(
            path, requestId, correlationId);
    }

    const std::string entityTag =
        vdrsuite::http::publicStrongEntityTag(
            resource.resourceRevision);
    if (entityTag.empty())
    {
        return serviceUnavailableProblem(
            path, requestId, correlationId);
    }

    const auto condition =
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

    const PublicAccountCredentialItem& credential =
        resource.credential;
    std::string body =
        "{\"accountId\":\"" +
        jsonEscape(resource.accountId) +
        "\",\"actorId\":\"" +
        jsonEscape(resource.actorId) +
        "\",\"credentialId\":\"" +
        jsonEscape(credential.credentialId) +
        "\",\"credentialType\":\"" +
        jsonEscape(credential.credentialType) +
        "\",\"active\":" +
        std::string(credential.active ? "true" : "false") +
        ",\"expired\":" +
        std::string(credential.expired ? "true" : "false") +
        ",\"revoked\":" +
        std::string(credential.revoked ? "true" : "false") +
        ",\"expiresAt\":";
    body += credential.expiresAt.empty()
        ? "null"
        : "\"" + jsonEscape(credential.expiresAt) + "\"";
    body +=
        ",\"createdAt\":\"" +
        jsonEscape(credential.createdAt) +
        "\",\"links\":{\"self\":\"" +
        jsonEscape(path) +
        "\"}}";

    ApiResponse response =
        jsonResponse(body, requestId, correlationId);
    response.headers["ETag"] = entityTag;
    return response;
}

ApiResponse publicAccountCredentialCollectionResponse(
    const PublicAccountCredentialCollectionResource& collection,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    std::string body =
        "{\"accountId\":\"" +
        jsonEscape(collection.accountId) +
        "\",\"actorId\":\"" +
        jsonEscape(collection.actorId) +
        "\",\"items\":[";
    for (std::size_t index = 0U;
         index < collection.credentials.size();
         ++index)
    {
        if (index > 0U) body += ",";
        const PublicAccountCredentialItem& credential =
            collection.credentials[index];
        body +=
            "{\"credentialId\":\"" +
            jsonEscape(credential.credentialId) +
            "\",\"credentialType\":\"" +
            jsonEscape(credential.credentialType) +
            "\",\"active\":" +
            std::string(credential.active ? "true" : "false") +
            ",\"expired\":" +
            std::string(credential.expired ? "true" : "false") +
            ",\"revoked\":" +
            std::string(credential.revoked ? "true" : "false") +
            ",\"expiresAt\":";
        body += credential.expiresAt.empty()
            ? "null"
            : "\"" + jsonEscape(credential.expiresAt) + "\"";
        body +=
            ",\"createdAt\":\"" +
            jsonEscape(credential.createdAt) +
            "\"}";
    }
    body +=
        "],\"links\":{\"self\":\"" +
        jsonEscape(path) +
        "\"}}";
    return jsonResponse(body, requestId, correlationId);
}

ApiResponse publicAccountSessionResourceResponse(
    const PublicAccountSessionResource& resource,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& ifNoneMatch)
{
    if (!publicSessionLifecycleRevision(
            resource.resourceRevision))
    {
        return serviceUnavailableProblem(
            path, requestId, correlationId);
    }

    const std::string entityTag =
        vdrsuite::http::publicStrongEntityTag(
            resource.resourceRevision);
    if (entityTag.empty())
    {
        return serviceUnavailableProblem(
            path, requestId, correlationId);
    }

    const auto condition =
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

    const PublicAccountSessionItem& session =
        resource.session;
    std::string body =
        "{\"accountId\":\"" +
        jsonEscape(resource.accountId) +
        "\",\"actorId\":\"" +
        jsonEscape(resource.actorId) +
        "\",\"sessionId\":\"" +
        jsonEscape(session.sessionId) +
        "\",\"deviceId\":\"" +
        jsonEscape(session.deviceId) +
        "\",\"issuedFromCredentialId\":\"" +
        jsonEscape(session.issuedFromCredentialId) +
        "\",\"active\":" +
        std::string(session.active ? "true" : "false") +
        ",\"expired\":" +
        std::string(session.expired ? "true" : "false") +
        ",\"revoked\":" +
        std::string(session.revoked ? "true" : "false") +
        ",\"expiresAt\":";
    body += session.expiresAt.empty()
        ? "null"
        : "\"" + jsonEscape(session.expiresAt) + "\"";
    body += ",\"lastSeenAt\":";
    body += session.lastSeenAt.empty()
        ? "null"
        : "\"" + jsonEscape(session.lastSeenAt) + "\"";
    body +=
        ",\"createdAt\":\"" +
        jsonEscape(session.createdAt) +
        "\",\"links\":{\"self\":\"" +
        jsonEscape(path) +
        "\"}}";

    ApiResponse response =
        jsonResponse(
            body,
            requestId,
            correlationId);
    response.headers["ETag"] = entityTag;
    return response;
}

ApiResponse publicAccountSessionCollectionResponse(
    const PublicAccountSessionCollectionResource& collection,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    std::string body =
        "{\"accountId\":\"" +
        jsonEscape(collection.accountId) +
        "\",\"actorId\":\"" +
        jsonEscape(collection.actorId) +
        "\",\"items\":[";
    for (std::size_t index = 0U;
         index < collection.sessions.size();
         ++index)
    {
        if (index > 0U) body += ",";
        const PublicAccountSessionItem& session =
            collection.sessions[index];
        body +=
            "{\"sessionId\":\"" +
            jsonEscape(session.sessionId) +
            "\",\"deviceId\":\"" +
            jsonEscape(session.deviceId) +
            "\",\"issuedFromCredentialId\":\"" +
            jsonEscape(session.issuedFromCredentialId) +
            "\",\"active\":" +
            std::string(session.active ? "true" : "false") +
            ",\"expired\":" +
            std::string(session.expired ? "true" : "false") +
            ",\"revoked\":" +
            std::string(session.revoked ? "true" : "false") +
            ",\"expiresAt\":";
        body += session.expiresAt.empty()
            ? "null"
            : "\"" + jsonEscape(session.expiresAt) + "\"";
        body += ",\"lastSeenAt\":";
        body += session.lastSeenAt.empty()
            ? "null"
            : "\"" + jsonEscape(session.lastSeenAt) + "\"";
        body +=
            ",\"createdAt\":\"" +
            jsonEscape(session.createdAt) +
            "\"}";
    }
    body +=
        "],\"links\":{\"self\":\"" +
        jsonEscape(path) +
        "\"}}";
    return jsonResponse(body, requestId, correlationId);
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

struct PublicRecordingCollectionQuery
{
    std::string backendId;
    std::size_t limit = 50U;
    std::string cursor;
};

bool parsePublicRecordingCollectionQuery(
    const std::string& target,
    PublicRecordingCollectionQuery& query)
{
    const std::string text = requestQueryString(target);
    if (text.empty()) return false;
    bool seenBackend = false, seenLimit = false, seenCursor = false;
    std::size_t start = 0U;
    while (start <= text.size())
    {
        const auto end = text.find('&', start);
        const auto field = text.substr(start, end == std::string::npos
            ? std::string::npos : end - start);
        const auto separator = field.find('=');
        if (separator == std::string::npos || field.empty()) return false;
        const auto name = field.substr(0, separator);
        const auto value = field.substr(separator + 1U);
        if (name == "backendId")
        {
            if (seenBackend || value.empty() || value.size() > 128U ||
                !std::all_of(value.begin(), value.end(),
                    [](unsigned char c) { return std::isalnum(c) ||
                        c == '.' || c == '_' || c == '-'; }))
                return false;
            query.backendId = value;
            seenBackend = true;
        }
        else if (name == "limit")
        {
            std::size_t parsed = 0U;
            if (seenLimit || !decimalSize(value, parsed) || parsed == 0U ||
                parsed > PublicRecordingMaximumLimit) return false;
            query.limit = parsed;
            seenLimit = true;
        }
        else if (name == "cursor")
        {
            if (seenCursor || value.empty() || value.size() > 1024U)
                return false;
            query.cursor = value;
            seenCursor = true;
        }
        else return false;
        if (end == std::string::npos) break;
        start = end + 1U;
    }
    return seenBackend;
}

std::string publicRecordingCursor(
    const std::string& backend,
    const VdrPublicRecordingSummary& item)
{
    std::string payload = backend;
    payload.push_back('\0');
    payload += item.recordedAt;
    payload.push_back('\0');
    payload += item.recordingId;
    return "rc1_" + hexEncode(payload);
}

bool decodePublicRecordingCursor(
    const std::string& cursor,
    const std::string& backend,
    std::string& recordedAt,
    std::string& recordingId)
{
    if (cursor.compare(0, 4, "rc1_") != 0) return false;
    std::string decoded;
    if (!hexDecode(cursor.substr(4), decoded)) return false;
    const auto first = decoded.find('\0');
    const auto second = decoded.find('\0', first == std::string::npos
        ? 0U : first + 1U);
    if (first == std::string::npos || second == std::string::npos ||
        decoded.find('\0', second + 1U) != std::string::npos ||
        decoded.substr(0, first) != backend) return false;
    recordedAt = decoded.substr(first + 1U, second - first - 1U);
    recordingId = decoded.substr(second + 1U);
    return !recordedAt.empty() && recordingId.size() == 36U &&
        recordingId.compare(0U, 4U, "rec_") == 0;
}

std::string publicRecordingTarget(
    const PublicRecordingCollectionQuery& query,
    const std::string& cursor)
{
    std::string target = std::string(PublicRecordingCollectionPath) +
        "?backendId=" + query.backendId + "&limit=" +
        std::to_string(query.limit);
    if (!cursor.empty()) target += "&cursor=" + cursor;
    return target;
}

ApiResponse publicRecordingCollectionResponse(
    const VdrPublicRecordingCollection& page,
    const PublicRecordingCollectionQuery& query,
    const std::string& nextCursor,
    const std::string& requestId,
    const std::string& correlationId)
{
    const std::string self = publicRecordingTarget(query, query.cursor);
    const std::string next = nextCursor.empty() ? std::string()
        : publicRecordingTarget(query, nextCursor);
    std::string body = "{\"items\":[";
    for (std::size_t i = 0; i < page.items.size(); ++i)
    {
        if (i != 0U) body += ",";
        const auto& item = page.items[i];
        body += "{\"recordingId\":\"" + jsonEscape(item.recordingId) +
            "\",\"backendId\":\"" + jsonEscape(item.backendId) +
            "\",\"title\":\"" + jsonEscape(item.title) +
            "\",\"recordedAt\":\"" + jsonEscape(item.recordedAt) +
            "\",\"durationKnown\":" +
            std::string(item.durationKnown ? "true" : "false") +
            ",\"durationSeconds\":" + std::to_string(item.durationSeconds) +
            "}";
    }
    body += "],\"page\":{\"limit\":" + std::to_string(query.limit) +
        ",\"nextCursor\":";
    body += nextCursor.empty() ? "null" :
        "\"" + jsonEscape(nextCursor) + "\"";
    body += ",\"hasMore\":";
    body += nextCursor.empty() ? "false" : "true";
    body += "},\"meta\":{\"partial\":false},\"links\":{\"self\":\"" +
        jsonEscape(self) + "\",\"next\":";
    body += next.empty() ? "null" :
        "\"" + jsonEscape(next) + "\"";
    body += "}}";
    return jsonResponse(body, requestId, correlationId);
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

void PublicApiRuntime::registerDevicePairingCreate(
    DevicePairingCreate create)
{
    std::lock_guard<std::mutex> lock(
        devicePairingCreateMutex_);
    devicePairingCreate_ = std::move(create);
}

void PublicApiRuntime::resetDevicePairingCreate()
{
    std::lock_guard<std::mutex> lock(
        devicePairingCreateMutex_);
    devicePairingCreate_ = {};
}

bool PublicApiRuntime::devicePairingCreateConfigured() const
{
    std::lock_guard<std::mutex> lock(
        devicePairingCreateMutex_);
    return static_cast<bool>(devicePairingCreate_);
}

void PublicApiRuntime::registerDevicePairingLookup(
    DevicePairingLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        devicePairingLookupMutex_);
    devicePairingLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetDevicePairingLookup()
{
    std::lock_guard<std::mutex> lock(
        devicePairingLookupMutex_);
    devicePairingLookup_ = {};
}

bool PublicApiRuntime::devicePairingLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        devicePairingLookupMutex_);
    return static_cast<bool>(devicePairingLookup_);
}

void PublicApiRuntime::registerDeviceCredentialIssue(
    DeviceCredentialIssue issue)
{
    std::lock_guard<std::mutex> lock(deviceCredentialIssueMutex_);
    deviceCredentialIssue_ = std::move(issue);
}

void PublicApiRuntime::resetDeviceCredentialIssue()
{
    std::lock_guard<std::mutex> lock(deviceCredentialIssueMutex_);
    deviceCredentialIssue_ = {};
}

bool PublicApiRuntime::deviceCredentialIssueConfigured() const
{
    std::lock_guard<std::mutex> lock(deviceCredentialIssueMutex_);
    return static_cast<bool>(deviceCredentialIssue_);
}

void PublicApiRuntime::registerDevicePairingAdministrationCollectionLookup(
    DevicePairingAdministrationCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationCollectionLookupMutex_);
    devicePairingAdministrationCollectionLookup_ =
        std::move(lookup);
}

void PublicApiRuntime::resetDevicePairingAdministrationCollectionLookup()
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationCollectionLookupMutex_);
    devicePairingAdministrationCollectionLookup_ = {};
}

bool PublicApiRuntime::
devicePairingAdministrationCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationCollectionLookupMutex_);
    return static_cast<bool>(
        devicePairingAdministrationCollectionLookup_);
}

void PublicApiRuntime::registerDevicePairingAdministrationLookup(
    DevicePairingAdministrationLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationLookupMutex_);
    devicePairingAdministrationLookup_ =
        std::move(lookup);
}

void PublicApiRuntime::resetDevicePairingAdministrationLookup()
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationLookupMutex_);
    devicePairingAdministrationLookup_ = {};
}

bool PublicApiRuntime::
devicePairingAdministrationLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        devicePairingAdministrationLookupMutex_);
    return static_cast<bool>(
        devicePairingAdministrationLookup_);
}

void PublicApiRuntime::registerDevicePairingDecision(
    DevicePairingDecision decision)
{
    std::lock_guard<std::mutex> lock(
        devicePairingDecisionMutex_);
    devicePairingDecision_ =
        std::move(decision);
}

void PublicApiRuntime::resetDevicePairingDecision()
{
    std::lock_guard<std::mutex> lock(
        devicePairingDecisionMutex_);
    devicePairingDecision_ = {};
}

bool PublicApiRuntime::devicePairingDecisionConfigured() const
{
    std::lock_guard<std::mutex> lock(
        devicePairingDecisionMutex_);
    return static_cast<bool>(devicePairingDecision_);
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

void PublicApiRuntime::registerDeviceLifecycleLookup(
    DeviceLifecycleLookup lookup)
{
    std::lock_guard<std::mutex> lock(deviceLifecycleLookupMutex_);
    deviceLifecycleLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetDeviceLifecycleLookup()
{
    std::lock_guard<std::mutex> lock(deviceLifecycleLookupMutex_);
    deviceLifecycleLookup_ = {};
}

void PublicApiRuntime::registerDeviceLifecycleMutation(
    DeviceLifecycleMutation mutation)
{
    std::lock_guard<std::mutex> lock(deviceLifecycleMutationMutex_);
    deviceLifecycleMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetDeviceLifecycleMutation()
{
    std::lock_guard<std::mutex> lock(deviceLifecycleMutationMutex_);
    deviceLifecycleMutation_ = {};
}

bool PublicApiRuntime::deviceLifecycleAdministrationConfigured() const
{
    std::lock_guard<std::mutex> a(deviceLifecycleLookupMutex_);
    std::lock_guard<std::mutex> b(deviceLifecycleMutationMutex_);
    return static_cast<bool>(deviceLifecycleLookup_) &&
           static_cast<bool>(deviceLifecycleMutation_);
}

void PublicApiRuntime::registerDeviceCredentialRotation(
    DeviceCredentialRotation rotate)
{
    std::lock_guard<std::mutex> lock(deviceCredentialRotationMutex_);
    deviceCredentialRotation_ = std::move(rotate);
}

void PublicApiRuntime::resetDeviceCredentialRotation()
{
    std::lock_guard<std::mutex> lock(deviceCredentialRotationMutex_);
    deviceCredentialRotation_ = {};
}

bool PublicApiRuntime::deviceCredentialRotationConfigured() const
{
    std::lock_guard<std::mutex> lock(deviceCredentialRotationMutex_);
    return static_cast<bool>(deviceCredentialRotation_);
}

void PublicApiRuntime::registerDeviceGrantLookup(
    DeviceGrantLookup lookup)
{
    std::lock_guard<std::mutex> lock(deviceGrantLookupMutex_);
    deviceGrantLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetDeviceGrantLookup()
{
    std::lock_guard<std::mutex> lock(deviceGrantLookupMutex_);
    deviceGrantLookup_ = {};
}

void PublicApiRuntime::registerDeviceGrantMutation(
    DeviceGrantMutation mutation)
{
    std::lock_guard<std::mutex> lock(deviceGrantMutationMutex_);
    deviceGrantMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetDeviceGrantMutation()
{
    std::lock_guard<std::mutex> lock(deviceGrantMutationMutex_);
    deviceGrantMutation_ = {};
}

bool PublicApiRuntime::deviceGrantAdministrationConfigured() const
{
    std::lock_guard<std::mutex> lookupLock(deviceGrantLookupMutex_);
    std::lock_guard<std::mutex> mutationLock(deviceGrantMutationMutex_);
    return static_cast<bool>(deviceGrantLookup_) &&
           static_cast<bool>(deviceGrantMutation_);
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

void PublicApiRuntime::registerAccountCredentialLookup(
    AccountCredentialLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountCredentialLookupMutex_);
    accountCredentialLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountCredentialLookup()
{
    std::lock_guard<std::mutex> lock(
        accountCredentialLookupMutex_);
    accountCredentialLookup_ = {};
}

bool PublicApiRuntime::accountCredentialLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountCredentialLookupMutex_);
    return static_cast<bool>(accountCredentialLookup_);
}

void PublicApiRuntime::registerAccountCredentialItemLookup(
    AccountCredentialItemLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountCredentialItemLookupMutex_);
    accountCredentialItemLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountCredentialItemLookup()
{
    std::lock_guard<std::mutex> lock(
        accountCredentialItemLookupMutex_);
    accountCredentialItemLookup_ = {};
}

bool PublicApiRuntime::accountCredentialItemLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountCredentialItemLookupMutex_);
    return static_cast<bool>(accountCredentialItemLookup_);
}

void PublicApiRuntime::registerAccountCredentialMutation(
    AccountCredentialMutation mutation)
{
    std::lock_guard<std::mutex> lock(
        accountCredentialMutationMutex_);
    accountCredentialMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetAccountCredentialMutation()
{
    std::lock_guard<std::mutex> lock(
        accountCredentialMutationMutex_);
    accountCredentialMutation_ = {};
}

bool PublicApiRuntime::accountCredentialMutationConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountCredentialMutationMutex_);
    return static_cast<bool>(accountCredentialMutation_);
}

void PublicApiRuntime::registerAccountSessionLookup(
    AccountSessionLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountSessionLookupMutex_);
    accountSessionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountSessionLookup()
{
    std::lock_guard<std::mutex> lock(
        accountSessionLookupMutex_);
    accountSessionLookup_ = {};
}

bool PublicApiRuntime::accountSessionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountSessionLookupMutex_);
    return static_cast<bool>(accountSessionLookup_);
}

void PublicApiRuntime::registerAccountSessionItemLookup(
    AccountSessionItemLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        accountSessionItemLookupMutex_);
    accountSessionItemLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetAccountSessionItemLookup()
{
    std::lock_guard<std::mutex> lock(
        accountSessionItemLookupMutex_);
    accountSessionItemLookup_ = {};
}

bool PublicApiRuntime::accountSessionItemLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountSessionItemLookupMutex_);
    return static_cast<bool>(accountSessionItemLookup_);
}

void PublicApiRuntime::registerAccountSessionMutation(
    AccountSessionMutation mutation)
{
    std::lock_guard<std::mutex> lock(
        accountSessionMutationMutex_);
    accountSessionMutation_ = std::move(mutation);
}

void PublicApiRuntime::resetAccountSessionMutation()
{
    std::lock_guard<std::mutex> lock(
        accountSessionMutationMutex_);
    accountSessionMutation_ = {};
}

bool PublicApiRuntime::accountSessionMutationConfigured() const
{
    std::lock_guard<std::mutex> lock(
        accountSessionMutationMutex_);
    return static_cast<bool>(accountSessionMutation_);
}

void PublicApiRuntime::registerRecordingCollectionLookup(
    RecordingCollectionLookup lookup)
{
    std::lock_guard<std::mutex> lock(recordingCollectionLookupMutex_);
    recordingCollectionLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetRecordingCollectionLookup()
{
    std::lock_guard<std::mutex> lock(recordingCollectionLookupMutex_);
    recordingCollectionLookup_ = {};
}

bool PublicApiRuntime::recordingCollectionLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(recordingCollectionLookupMutex_);
    return static_cast<bool>(recordingCollectionLookup_);
}

VdrPublicRecordingCollection PublicApiRuntime::lookupRecordingCollection(
    const std::string& backendId) const
{
    RecordingCollectionLookup lookup;
    {
        std::lock_guard<std::mutex> lock(recordingCollectionLookupMutex_);
        lookup = recordingCollectionLookup_;
    }
    if (!lookup) return {{}, false};
    return lookup(backendId);
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
    const std::vector<std::string>& authorizedBackendIds,
    const std::string& pairingToken) const
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
            deviceGrantAdministrationConfigured(),
            deviceLifecycleAdministrationConfigured(),
            deviceCredentialRotationConfigured(),
            accountCredentialLookupConfigured() &&
                accountSessionLookupConfigured(),
            accountCredentialItemLookupConfigured() &&
                accountCredentialMutationConfigured(),
            accountSessionItemLookupConfigured() &&
                accountSessionMutationConfigured(),
            devicePairingCreateConfigured() &&
                devicePairingLookupConfigured(),
            devicePairingAdministrationCollectionLookupConfigured() &&
                devicePairingAdministrationLookupConfigured() &&
                devicePairingDecisionConfigured(),
            deviceCredentialIssueConfigured(),
            requestId,
            correlationId);
        return true;
    }

    if (path == PublicDevicePairingCollectionPath)
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        PublicDevicePairingCollectionQuery query;
        if (!parsePublicDevicePairingCollectionQuery(
                requestTarget,
                query))
        {
            response = invalidRequestProblem(
                path,
                "The Device Pairing collection query is invalid.",
                requestId,
                correlationId);
            return true;
        }

        std::string afterPairingRequestId;
        if (!query.cursor.empty() &&
            !decodePublicDevicePairingCursor(
                query.cursor,
                afterPairingRequestId))
        {
            response = invalidRequestProblem(
                path,
                "The Device Pairing collection cursor is invalid.",
                requestId,
                correlationId);
            return true;
        }

        DevicePairingAdministrationCollectionLookup lookup;
        {
            std::lock_guard<std::mutex> lock(
                devicePairingAdministrationCollectionLookupMutex_);
            lookup =
                devicePairingAdministrationCollectionLookup_;
        }
        if (!lookup)
        {
            response = serviceUnavailableProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        PublicDevicePairingAdministrationCollectionRequest request;
        request.afterPairingRequestId =
            afterPairingRequestId;
        request.limit = query.limit;
        const PublicDevicePairingAdministrationCollectionResult page =
            lookup(request);

        switch (page.status)
        {
            case PublicDevicePairingAdministrationStatus::ok:
            {
                if (page.requests.size() > query.limit ||
                    (page.hasMore &&
                     page.requests.size() != query.limit))
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                    return true;
                }

                std::string previous = afterPairingRequestId;
                for (const auto& item : page.requests)
                {
                    const std::string& id =
                        item.resource.pairingRequestId;
                    if (id.empty() ||
                        (!previous.empty() &&
                         id <= previous) ||
                        item.resource.state != "pending" ||
                        item.resource.expiresAt.empty() ||
                        item.resource.pollIntervalSeconds <= 0 ||
                        item.resource.client.displayName.empty() ||
                        item.resource.client.clientKind.empty() ||
                        !publicDevicePairingRevision(
                            item.resourceRevision,
                            id) ||
                        !item.decidedByActorId.empty() ||
                        !item.decidedAt.empty())
                    {
                        response = serviceUnavailableProblem(
                            path,
                            requestId,
                            correlationId);
                        return true;
                    }
                    previous = id;
                }

                response =
                    publicDevicePairingAdministrationCollectionResponse(
                        page,
                        query,
                        requestId,
                        correlationId);
                return true;
            }

            case PublicDevicePairingAdministrationStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The Device Pairing administration request is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicDevicePairingAdministrationStatus::notFound:
            case PublicDevicePairingAdministrationStatus::expired:
            case PublicDevicePairingAdministrationStatus::revisionConflict:
            case PublicDevicePairingAdministrationStatus::stateConflict:
            case PublicDevicePairingAdministrationStatus::unavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
    }

    std::string pairingRequestId;
    if (publicDevicePairingPath(
            path,
            pairingRequestId))
    {
        if (requestTarget != path)
        {
            response = invalidRequestProblem(
                path,
                pairingToken.empty()
                    ? "Device Pairing administration item reads do not accept query parameters."
                    : "Device Pairing polling does not accept query parameters.",
                requestId,
                correlationId);
            return true;
        }

        if (!pairingToken.empty())
        {
            DevicePairingLookup lookup;
            {
                std::lock_guard<std::mutex> lock(
                    devicePairingLookupMutex_);
                lookup = devicePairingLookup_;
            }
            if (!lookup)
            {
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
            }

            PublicDevicePairingLookupRequest lookupRequest;
            lookupRequest.pairingRequestId =
                pairingRequestId;
            lookupRequest.pairingToken =
                pairingToken;
            const PublicDevicePairingLookupResult found =
                lookup(lookupRequest);

            switch (found.status)
            {
                case PublicDevicePairingLookupStatus::ok:
                    if (found.resource.pairingRequestId !=
                            pairingRequestId ||
                        (found.resource.state != "pending" &&
                         found.resource.state != "approved" &&
                         found.resource.state != "rejected") ||
                        found.resource.expiresAt.empty() ||
                        found.resource.pollIntervalSeconds <= 0 ||
                        found.resource.client.displayName.empty() ||
                        found.resource.client.clientKind.empty())
                    {
                        response = serviceUnavailableProblem(
                            path,
                            requestId,
                            correlationId);
                    }
                    else
                    {
                        response =
                            publicDevicePairingResponse(
                                found.resource,
                                path,
                                requestId,
                                correlationId);
                    }
                    return true;

                case PublicDevicePairingLookupStatus::invalid:
                    response = invalidRequestProblem(
                        path,
                        "The Device Pairing request identifier is invalid.",
                        requestId,
                        correlationId);
                    return true;

                case PublicDevicePairingLookupStatus::notFound:
                    response = notFoundProblem(
                        path,
                        requestId,
                        correlationId);
                    return true;

                case PublicDevicePairingLookupStatus::unauthorized:
                    response = problemResponse(
                        401,
                        "unauthorized",
                        "Unauthorized",
                        "The Device Pairing polling token is invalid.",
                        path,
                        requestId,
                        correlationId);
                    return true;

                case PublicDevicePairingLookupStatus::expired:
                    response = problemResponse(
                        410,
                        "pairing_expired",
                        "Pairing request expired",
                        "The Device Pairing request has expired.",
                        path,
                        requestId,
                        correlationId);
                    return true;

                case PublicDevicePairingLookupStatus::consumed:
                    response = problemResponse(
                        410, "pairing_consumed",
                        "Pairing already consumed",
                        "A new pairing is required.",
                        path, requestId, correlationId);
                    return true;

                case PublicDevicePairingLookupStatus::unavailable:
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                    return true;
            }
        }

        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        DevicePairingAdministrationLookup lookup;
        {
            std::lock_guard<std::mutex> lock(
                devicePairingAdministrationLookupMutex_);
            lookup =
                devicePairingAdministrationLookup_;
        }
        if (!lookup)
        {
            response = serviceUnavailableProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        const PublicDevicePairingAdministrationLookupResult found =
            lookup(pairingRequestId);
        switch (found.status)
        {
            case PublicDevicePairingAdministrationStatus::ok:
                if (found.request.resource.pairingRequestId !=
                        pairingRequestId ||
                    (found.request.resource.state != "pending" &&
                     found.request.resource.state != "approved" &&
                     found.request.resource.state != "rejected") ||
                    found.request.resource.expiresAt.empty() ||
                    found.request.resource.pollIntervalSeconds <= 0 ||
                    found.request.resource.client.displayName.empty() ||
                    found.request.resource.client.clientKind.empty() ||
                    !publicDevicePairingRevision(
                        found.request.resourceRevision,
                        pairingRequestId) ||
                    (found.request.resource.state == "pending" &&
                     (!found.request.decidedAt.empty() ||
                      !found.request.decidedByActorId.empty())) ||
                    (found.request.resource.state != "pending" &&
                     (found.request.decidedAt.empty() ||
                      found.request.decidedByActorId.empty())))
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                }
                else
                {
                    response =
                        publicDevicePairingAdministrationResponse(
                            found.request,
                            path,
                            requestId,
                            correlationId);
                }
                return true;

            case PublicDevicePairingAdministrationStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The Device Pairing request identifier is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicDevicePairingAdministrationStatus::notFound:
                response = notFoundProblem(
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicDevicePairingAdministrationStatus::expired:
                response = problemResponse(
                    410,
                    "pairing_expired",
                    "Pairing request expired",
                    "The Device Pairing request has expired.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicDevicePairingAdministrationStatus::revisionConflict:
            case PublicDevicePairingAdministrationStatus::stateConflict:
            case PublicDevicePairingAdministrationStatus::unavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
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
    std::string credentialId;
    std::string sessionId;

    if (publicAccountCredentialItemPath(
            path, accountId, credentialId))
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
                "Account Credential item read does not accept query parameters.",
                requestId,
                correlationId);
            return true;
        }

        AccountCredentialItemLookup lookup;
        {
            std::lock_guard<std::mutex> lock(
                accountCredentialItemLookupMutex_);
            lookup = accountCredentialItemLookup_;
        }
        if (!lookup)
        {
            response = serviceUnavailableProblem(
                path, requestId, correlationId);
            return true;
        }

        const PublicAccountCredentialLookupResult found =
            lookup(accountId, credentialId);
        switch (found.status)
        {
            case PublicAccountCredentialAdministrationStatus::ok:
                if (found.resource.accountId != accountId ||
                    found.resource.actorId.empty() ||
                    found.resource.credential.credentialId != credentialId ||
                    found.resource.credential.credentialType.empty() ||
                    found.resource.credential.createdAt.empty() ||
                    !publicCredentialLifecycleRevision(
                        found.resource.resourceRevision))
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                response =
                    publicAccountCredentialResourceResponse(
                        found.resource,
                        path,
                        requestId,
                        correlationId,
                        ifNoneMatch);
                return true;

            case PublicAccountCredentialAdministrationStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The Account or Credential identifier is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicAccountCredentialAdministrationStatus::notFound:
                response = notFoundProblem(
                    path, requestId, correlationId);
                return true;

            case PublicAccountCredentialAdministrationStatus::validationError:
            case PublicAccountCredentialAdministrationStatus::revisionConflict:
            case PublicAccountCredentialAdministrationStatus::finalAdministrator:
            case PublicAccountCredentialAdministrationStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (publicAccountSessionItemPath(
            path, accountId, sessionId))
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
                "Account Session item read does not accept query parameters.",
                requestId,
                correlationId);
            return true;
        }

        AccountSessionItemLookup lookup;
        {
            std::lock_guard<std::mutex> lock(
                accountSessionItemLookupMutex_);
            lookup = accountSessionItemLookup_;
        }
        if (!lookup)
        {
            response = serviceUnavailableProblem(
                path, requestId, correlationId);
            return true;
        }

        const PublicAccountSessionLookupResult found =
            lookup(accountId, sessionId);
        switch (found.status)
        {
            case PublicAccountSessionAdministrationStatus::ok:
                if (found.resource.accountId != accountId ||
                    found.resource.actorId.empty() ||
                    found.resource.session.sessionId != sessionId ||
                    found.resource.session.deviceId.empty() ||
                    found.resource.session.issuedFromCredentialId.empty() ||
                    found.resource.session.createdAt.empty() ||
                    !publicSessionLifecycleRevision(
                        found.resource.resourceRevision))
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                response =
                    publicAccountSessionResourceResponse(
                        found.resource,
                        path,
                        requestId,
                        correlationId,
                        ifNoneMatch);
                return true;

            case PublicAccountSessionAdministrationStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The Account or Session identifier is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicAccountSessionAdministrationStatus::notFound:
                response = notFoundProblem(
                    path, requestId, correlationId);
                return true;

            case PublicAccountSessionAdministrationStatus::revisionConflict:
            case PublicAccountSessionAdministrationStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (publicAccountCredentialPath(path, accountId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(path, requestId, correlationId);
            return true;
        }
        if (requestTarget != path)
        {
            response = invalidRequestProblem(
                path,
                "Account Credential metadata read does not accept query parameters.",
                requestId,
                correlationId);
            return true;
        }
        AccountCredentialLookup lookup;
        {
            std::lock_guard<std::mutex> lock(
                accountCredentialLookupMutex_);
            lookup = accountCredentialLookup_;
        }
        if (!lookup)
        {
            response = serviceUnavailableProblem(
                path, requestId, correlationId);
            return true;
        }
        const PublicAccountCredentialCollectionResult found =
            lookup(accountId);
        switch (found.status)
        {
            case PublicAccountSecurityMetadataStatus::ok:
                if (found.collection.accountId != accountId ||
                    found.collection.actorId.empty())
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                for (const PublicAccountCredentialItem& credential :
                     found.collection.credentials)
                {
                    if (credential.credentialId.empty() ||
                        credential.credentialType.empty() ||
                        credential.credentialType == "browser-session" ||
                        credential.createdAt.empty())
                    {
                        response = serviceUnavailableProblem(
                            path, requestId, correlationId);
                        return true;
                    }
                }
                response = publicAccountCredentialCollectionResponse(
                    found.collection, path, requestId, correlationId);
                return true;
            case PublicAccountSecurityMetadataStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The Account identifier is invalid.",
                    requestId,
                    correlationId);
                return true;
            case PublicAccountSecurityMetadataStatus::notFound:
                response = notFoundProblem(
                    path, requestId, correlationId);
                return true;
            case PublicAccountSecurityMetadataStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (publicAccountSessionPath(path, accountId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(path, requestId, correlationId);
            return true;
        }
        if (requestTarget != path)
        {
            response = invalidRequestProblem(
                path,
                "Account Session metadata read does not accept query parameters.",
                requestId,
                correlationId);
            return true;
        }
        AccountSessionLookup lookup;
        {
            std::lock_guard<std::mutex> lock(
                accountSessionLookupMutex_);
            lookup = accountSessionLookup_;
        }
        if (!lookup)
        {
            response = serviceUnavailableProblem(
                path, requestId, correlationId);
            return true;
        }
        const PublicAccountSessionCollectionResult found =
            lookup(accountId);
        switch (found.status)
        {
            case PublicAccountSecurityMetadataStatus::ok:
                if (found.collection.accountId != accountId ||
                    found.collection.actorId.empty())
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                for (const PublicAccountSessionItem& session :
                     found.collection.sessions)
                {
                    if (session.sessionId.empty() ||
                        session.deviceId.empty() ||
                        session.issuedFromCredentialId.empty() ||
                        session.createdAt.empty())
                    {
                        response = serviceUnavailableProblem(
                            path, requestId, correlationId);
                        return true;
                    }
                }
                response = publicAccountSessionCollectionResponse(
                    found.collection, path, requestId, correlationId);
                return true;
            case PublicAccountSecurityMetadataStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The Account identifier is invalid.",
                    requestId,
                    correlationId);
                return true;
            case PublicAccountSecurityMetadataStatus::notFound:
                response = notFoundProblem(
                    path, requestId, correlationId);
                return true;
            case PublicAccountSecurityMetadataStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    std::string lifecycleDeviceId;
    std::string lifecycleCredentialId;
    if (publicDeviceLifecyclePath(
            path, lifecycleDeviceId, lifecycleCredentialId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(path, requestId, correlationId);
            return true;
        }
        if (requestTarget != path)
        {
            response = invalidRequestProblem(
                path, "Device lifecycle GET does not accept query.",
                requestId, correlationId);
            return true;
        }
        DeviceLifecycleLookup lookup;
        {
            std::lock_guard<std::mutex> lock(deviceLifecycleLookupMutex_);
            lookup = deviceLifecycleLookup_;
        }
        if (!lookup)
        {
            response = serviceUnavailableProblem(path, requestId, correlationId);
            return true;
        }
        const PublicDeviceLifecycleResult found =
            lookup(lifecycleDeviceId, lifecycleCredentialId);
        switch (found.status)
        {
            case PublicDeviceLifecycleStatus::ok:
                if (found.resource.deviceId != lifecycleDeviceId ||
                    found.resource.credentialId != lifecycleCredentialId ||
                    found.resource.actorId.empty() ||
                    found.resource.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                response = publicDeviceLifecycleResponse(
                    found.resource, path, requestId, correlationId, ifNoneMatch);
                return true;
            case PublicDeviceLifecycleStatus::invalid:
                response = invalidRequestProblem(
                    path, "Invalid Device lifecycle target.",
                    requestId, correlationId);
                return true;
            case PublicDeviceLifecycleStatus::notFound:
                response = notFoundProblem(path, requestId, correlationId);
                return true;
            case PublicDeviceLifecycleStatus::revisionConflict:
            case PublicDeviceLifecycleStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    std::string deviceGrantDeviceId;
    if (publicDeviceGrantPath(path, deviceGrantDeviceId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(path, requestId, correlationId);
            return true;
        }
        if (requestTarget != path)
        {
            response = invalidRequestProblem(
                path, "Device Grant read takes no query parameters.",
                requestId, correlationId);
            return true;
        }

        DeviceGrantLookup lookup;
        {
            std::lock_guard<std::mutex> lock(deviceGrantLookupMutex_);
            lookup = deviceGrantLookup_;
        }
        if (!lookup)
        {
            response = serviceUnavailableProblem(path, requestId, correlationId);
            return true;
        }
        const PublicDeviceGrantLookupResult found = lookup(deviceGrantDeviceId);
        switch (found.status)
        {
            case PublicDeviceGrantStatus::ok:
                if (found.grantSet.deviceId != deviceGrantDeviceId ||
                    found.grantSet.actorId.empty() ||
                    !publicGrantSetRevision(found.grantSet.resourceRevision))
                {
                    response = serviceUnavailableProblem(path, requestId, correlationId);
                    return true;
                }
                response = publicDeviceGrantSetResponse(
                    found.grantSet, path, requestId, correlationId, ifNoneMatch);
                return true;
            case PublicDeviceGrantStatus::invalid:
                response = invalidRequestProblem(
                    path, "Invalid Device Grant request.",
                    requestId, correlationId);
                return true;
            case PublicDeviceGrantStatus::notFound:
                response = notFoundProblem(path, requestId, correlationId);
                return true;
            case PublicDeviceGrantStatus::revisionConflict:
            case PublicDeviceGrantStatus::unavailable:
                response = serviceUnavailableProblem(path, requestId, correlationId);
                return true;
        }
    }

    if (publicAccountGrantPath(path, accountId))
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
                "Account Grant-set read does not accept query parameters.",
                requestId,
                correlationId);
            return true;
        }

        AccountGrantLookup lookup;
        {
            std::lock_guard<std::mutex> lock(
                accountGrantLookupMutex_);
            lookup = accountGrantLookup_;
        }
        if (!lookup)
        {
            response = serviceUnavailableProblem(
                path, requestId, correlationId);
            return true;
        }

        const PublicAccountGrantLookupResult found =
            lookup(accountId);

        switch (found.status)
        {
            case PublicAccountGrantStatus::ok:
                if (found.grantSet.accountId != accountId ||
                    found.grantSet.actorId.empty() ||
                    !publicGrantSetRevision(
                        found.grantSet.resourceRevision))
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                for (const PublicAccountGrantItem& grant :
                     found.grantSet.grants)
                {
                    if (grant.permission.empty() ||
                        grant.backendId.empty())
                    {
                        response = serviceUnavailableProblem(
                            path, requestId, correlationId);
                        return true;
                    }
                }
                response = publicAccountGrantSetResponse(
                    found.grantSet,
                    path,
                    requestId,
                    correlationId,
                    ifNoneMatch);
                return true;

            case PublicAccountGrantStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The Account Grant-set request is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicAccountGrantStatus::notFound:
                response = notFoundProblem(
                    path, requestId, correlationId);
                return true;

            case PublicAccountGrantStatus::revisionConflict:
            case PublicAccountGrantStatus::finalAdministrator:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;

            case PublicAccountGrantStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

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
    const std::string& authorizedBackendId,
    const std::string& pairingToken) const
{
    const std::string path = requestPath(requestTarget);
    std::string operationId;
    std::string timerAssignmentId;
    std::string accountId;
    std::string credentialId;
    std::string sessionId;
    std::string pairingRequestId;

    const std::string issueSuffix = "/credential";
    const std::string pairingPrefix(PublicDevicePairingPrefix);
    if (path.compare(0U, pairingPrefix.size(), pairingPrefix) == 0 &&
        path.size() > pairingPrefix.size() + issueSuffix.size() &&
        path.compare(path.size() - issueSuffix.size(),
                     issueSuffix.size(), issueSuffix) == 0)
    {
        const std::string id = path.substr(
            pairingPrefix.size(),
            path.size() - pairingPrefix.size() - issueSuffix.size());
        if (id.find('/') == std::string::npos && !id.empty())
        {
            if (requestTarget != path || !body.empty() ||
                !ifMatch.empty() || !idempotencyKey.empty())
            {
                response = invalidRequestProblem(
                    path, "Credential issuance takes no body, query, or mutation preconditions.",
                    requestId, correlationId);
                return true;
            }
            if (pairingToken.empty())
            {
                response = unauthorizedProblem(path, requestId, correlationId);
                return true;
            }
            DeviceCredentialIssue handler;
            {
                std::lock_guard<std::mutex> lock(deviceCredentialIssueMutex_);
                handler = deviceCredentialIssue_;
            }
            if (!handler)
            {
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
            }
            PublicDeviceCredentialIssueRequest request;
            request.pairingRequestId = id;
            request.pairingToken = pairingToken;
            request.requestId = requestId;
            request.correlationId = correlationId;
            PublicDeviceCredentialIssueResult issued = handler(request);
            switch (issued.status)
            {
                case PublicDeviceCredentialIssueStatus::issued:
                    if (issued.actorId.empty() || issued.deviceId.empty() ||
                        issued.credentialId.empty() ||
                        issued.credentialSecret.empty())
                    {
                        response = serviceUnavailableProblem(
                            path, requestId, correlationId);
                    }
                    else
                    {
                        response = jsonResponse(
                            "{\"actorId\":\"" + jsonEscape(issued.actorId) +
                            "\",\"deviceId\":\"" + jsonEscape(issued.deviceId) +
                            "\",\"credentialId\":\"" + jsonEscape(issued.credentialId) +
                            "\",\"credentialSecret\":\"" +
                            jsonEscape(issued.credentialSecret) + "\"}",
                            requestId, correlationId);
                        response.statusCode = 201;
                        response.headers["Cache-Control"] = "no-store";
                    }
                    std::fill(issued.credentialSecret.begin(),
                              issued.credentialSecret.end(), '\0');
                    issued.credentialSecret.clear();
                    return true;
                case PublicDeviceCredentialIssueStatus::invalid:
                    response = invalidRequestProblem(path,
                        "Invalid pairing issuance request.",
                        requestId, correlationId);
                    return true;
                case PublicDeviceCredentialIssueStatus::notFound:
                    response = notFoundProblem(path, requestId, correlationId);
                    return true;
                case PublicDeviceCredentialIssueStatus::unauthorized:
                    response = unauthorizedProblem(path, requestId, correlationId);
                    return true;
                case PublicDeviceCredentialIssueStatus::notApproved:
                case PublicDeviceCredentialIssueStatus::consumed:
                    response = problemResponse(409, "pairing_state_conflict",
                        "Pairing state conflict",
                        "Pairing is not approved or already consumed.",
                        path, requestId, correlationId);
                    return true;
                case PublicDeviceCredentialIssueStatus::expired:
                    response = problemResponse(410, "pairing_expired",
                        "Pairing expired", "Pairing request has expired.",
                        path, requestId, correlationId);
                    return true;
                case PublicDeviceCredentialIssueStatus::unavailable:
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
            }
        }
    }

    if (path == PublicDevicePairingCollectionPath)
    {
        if (requestTarget != path)
        {
            response = invalidRequestProblem(
                path,
                "Device Pairing creation does not accept query parameters.",
                requestId,
                correlationId);
            return true;
        }

        if (!ifMatch.empty() ||
            !idempotencyKey.empty())
        {
            response = invalidRequestProblem(
                path,
                "Device Pairing creation does not accept If-Match or Idempotency-Key.",
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
                "Device Pairing creation requires Content-Type application/json.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (body.size() > 2048U)
        {
            response = invalidRequestProblem(
                path,
                "The Device Pairing request body is too large.",
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
                "The Device Pairing request body is not valid JSON.",
                path,
                requestId,
                correlationId);
            return true;
        }

        PublicDevicePairingCreateRequest createRequest;
        if (!parsePublicDevicePairingCreateBody(
                body,
                createRequest.client.displayName,
                createRequest.client.clientKind,
                createRequest.client.appVersion))
        {
            response = problemResponse(
                422,
                "validation_error",
                "Validation failed",
                "Device Pairing requires displayName and clientKind string fields, optional appVersion, and no unknown fields.",
                path,
                requestId,
                correlationId);
            return true;
        }
        createRequest.requestId = requestId;
        createRequest.correlationId =
            correlationId;

        DevicePairingCreate create;
        {
            std::lock_guard<std::mutex> lock(
                devicePairingCreateMutex_);
            create = devicePairingCreate_;
        }
        if (!create)
        {
            response = serviceUnavailableProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        const PublicDevicePairingCreateResult created =
            create(createRequest);
        switch (created.status)
        {
            case PublicDevicePairingCreateStatus::created:
                if (created.resource.pairingRequestId.empty() ||
                    created.resource.state != "pending" ||
                    created.resource.expiresAt.empty() ||
                    created.resource.pollIntervalSeconds <= 0 ||
                    created.resource.client.displayName.empty() ||
                    created.resource.client.clientKind.empty() ||
                    created.userCode.empty() ||
                    created.pairingToken.empty())
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                }
                else
                {
                    response =
                        publicDevicePairingCreatedResponse(
                            created,
                            requestId,
                            correlationId);
                }
                return true;

            case PublicDevicePairingCreateStatus::invalid:
                response = problemResponse(
                    422,
                    "validation_error",
                    "Validation failed",
                    "The Device Pairing presentation metadata is invalid.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicDevicePairingCreateStatus::entropyUnavailable:
            case PublicDevicePairingCreateStatus::hashingUnavailable:
            case PublicDevicePairingCreateStatus::unavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
    }

    if (publicDevicePairingPath(
            path,
            pairingRequestId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        if (requestTarget != path)
        {
            response = invalidRequestProblem(
                path,
                "Device Pairing decisions do not accept query parameters.",
                requestId,
                correlationId);
            return true;
        }

        if (!idempotencyKey.empty())
        {
            response = invalidRequestProblem(
                path,
                "Device Pairing decisions do not accept Idempotency-Key.",
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
                "Device Pairing decisions require Content-Type application/json.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (body.size() > 1024U)
        {
            response = invalidRequestProblem(
                path,
                "The Device Pairing decision body is too large.",
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
                "The Device Pairing decision body is not valid JSON.",
                path,
                requestId,
                correlationId);
            return true;
        }

        std::string decision;
        if (!parsePublicDevicePairingDecisionBody(
                body,
                decision))
        {
            response = problemResponse(
                422,
                "validation_error",
                "Validation failed",
                "Device Pairing decisions require exactly one decision field with approve or reject.",
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
                "Device Pairing decisions require one strong If-Match entity tag.",
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

        if (!publicDevicePairingRevision(
                expectedResourceRevision,
                pairingRequestId))
        {
            response = invalidRequestProblem(
                path,
                "If-Match does not identify this Device Pairing request revision.",
                requestId,
                correlationId);
            return true;
        }

        DevicePairingDecision mutation;
        {
            std::lock_guard<std::mutex> lock(
                devicePairingDecisionMutex_);
            mutation = devicePairingDecision_;
        }
        if (!mutation)
        {
            response = serviceUnavailableProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        PublicDevicePairingDecisionRequest mutationRequest;
        mutationRequest.actorRef = actorRef;
        mutationRequest.pairingRequestId =
            pairingRequestId;
        mutationRequest.expectedResourceRevision =
            expectedResourceRevision;
        mutationRequest.decision = decision;
        mutationRequest.requestId = requestId;
        mutationRequest.correlationId =
            correlationId;

        const PublicDevicePairingDecisionResult mutated =
            mutation(mutationRequest);
        switch (mutated.status)
        {
            case PublicDevicePairingAdministrationStatus::ok:
                if (mutated.request.resource.pairingRequestId !=
                        pairingRequestId ||
                    (mutated.request.resource.state != "approved" &&
                     mutated.request.resource.state != "rejected") ||
                    mutated.request.resource.expiresAt.empty() ||
                    mutated.request.resource.pollIntervalSeconds <= 0 ||
                    mutated.request.resource.client.displayName.empty() ||
                    mutated.request.resource.client.clientKind.empty() ||
                    mutated.request.decidedByActorId.empty() ||
                    mutated.request.decidedAt.empty() ||
                    !publicDevicePairingRevision(
                        mutated.request.resourceRevision,
                        pairingRequestId) ||
                    mutated.request.resourceRevision ==
                        expectedResourceRevision)
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                }
                else
                {
                    response =
                        publicDevicePairingAdministrationResponse(
                            mutated.request,
                            path,
                            requestId,
                            correlationId);
                }
                return true;

            case PublicDevicePairingAdministrationStatus::invalid:
                response = problemResponse(
                    422,
                    "validation_error",
                    "Validation failed",
                    "The Device Pairing decision is invalid.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicDevicePairingAdministrationStatus::notFound:
                response = notFoundProblem(
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicDevicePairingAdministrationStatus::expired:
                response = problemResponse(
                    410,
                    "pairing_expired",
                    "Pairing request expired",
                    "The Device Pairing request has expired.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicDevicePairingAdministrationStatus::revisionConflict:
                response = problemResponse(
                    412,
                    "revision_conflict",
                    "Revision conflict",
                    "The Device Pairing request changed after it was read.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicDevicePairingAdministrationStatus::stateConflict:
                response = problemResponse(
                    409,
                    "operation_conflict",
                    "Operation conflict",
                    "The Device Pairing request has already been decided.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicDevicePairingAdministrationStatus::unavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
    }

    if (publicAccountCredentialItemPath(
            path, accountId, credentialId))
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
                "Account Credential revoke does not accept query parameters.",
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
                "Account Credential revoke requires Content-Type application/json.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (body.size() > 1024U)
        {
            response = invalidRequestProblem(
                path,
                "The Account Credential revoke request body is too large.",
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
                "The Account Credential revoke request body is not valid JSON.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (!emptyJsonObject(body))
        {
            response = problemResponse(
                422,
                "validation_error",
                "Validation failed",
                "Account Credential revoke requires an empty JSON object.",
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
                "Account Credential revoke requires one strong If-Match entity tag.",
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

        if (!publicCredentialLifecycleRevision(
                expectedResourceRevision))
        {
            response = invalidRequestProblem(
                path,
                "If-Match does not identify an Account Credential lifecycle revision.",
                requestId,
                correlationId);
            return true;
        }

        AccountCredentialMutation mutation;
        {
            std::lock_guard<std::mutex> lock(
                accountCredentialMutationMutex_);
            mutation = accountCredentialMutation_;
        }
        if (!mutation)
        {
            response = serviceUnavailableProblem(
                path, requestId, correlationId);
            return true;
        }

        PublicAccountCredentialMutationRequest mutationRequest;
        mutationRequest.actorRef = actorRef;
        mutationRequest.accountId = accountId;
        mutationRequest.credentialId = credentialId;
        mutationRequest.expectedResourceRevision =
            expectedResourceRevision;
        mutationRequest.requestId = requestId;
        mutationRequest.correlationId = correlationId;

        const PublicAccountCredentialMutationResult mutated =
            mutation(mutationRequest);

        switch (mutated.status)
        {
            case PublicAccountCredentialAdministrationStatus::ok:
                if (mutated.resource.accountId != accountId ||
                    mutated.resource.actorId.empty() ||
                    mutated.resource.credential.credentialId != credentialId ||
                    mutated.resource.credential.credentialType.empty() ||
                    mutated.resource.credential.createdAt.empty() ||
                    !publicCredentialLifecycleRevision(
                        mutated.resource.resourceRevision))
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                response =
                    publicAccountCredentialResourceResponse(
                        mutated.resource,
                        path,
                        requestId,
                        correlationId,
                        "");
                return true;

            case PublicAccountCredentialAdministrationStatus::invalid:
                response = problemResponse(
                    422,
                    "validation_error",
                    "Validation failed",
                    "The Account Credential revoke request is invalid.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountCredentialAdministrationStatus::validationError:
                response = problemResponse(
                    422,
                    "validation_error",
                    "Validation failed",
                    "Only Human Account human-password credentials are revocable in this API slice.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountCredentialAdministrationStatus::notFound:
                response = notFoundProblem(
                    path, requestId, correlationId);
                return true;

            case PublicAccountCredentialAdministrationStatus::revisionConflict:
                response = problemResponse(
                    412,
                    "revision_conflict",
                    "Resource revision conflict",
                    "The Account Credential lifecycle changed after it was read.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountCredentialAdministrationStatus::finalAdministrator:
                response = problemResponse(
                    409,
                    "operation_conflict",
                    "Operation conflict",
                    "The final usable administrator credential cannot be revoked.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountCredentialAdministrationStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (publicAccountSessionItemPath(
            path, accountId, sessionId))
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
                "Account Session revoke does not accept query parameters.",
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
                "Account Session revoke requires Content-Type application/json.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (body.size() > 1024U)
        {
            response = invalidRequestProblem(
                path,
                "The Account Session revoke request body is too large.",
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
                "The Account Session revoke request body is not valid JSON.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (!emptyJsonObject(body))
        {
            response = problemResponse(
                422,
                "validation_error",
                "Validation failed",
                "Account Session revoke requires an empty JSON object.",
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
                "Account Session revoke requires one strong If-Match entity tag.",
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

        if (!publicSessionLifecycleRevision(
                expectedResourceRevision))
        {
            response = invalidRequestProblem(
                path,
                "If-Match does not identify an Account Session lifecycle revision.",
                requestId,
                correlationId);
            return true;
        }

        AccountSessionMutation mutation;
        {
            std::lock_guard<std::mutex> lock(
                accountSessionMutationMutex_);
            mutation = accountSessionMutation_;
        }
        if (!mutation)
        {
            response = serviceUnavailableProblem(
                path, requestId, correlationId);
            return true;
        }

        PublicAccountSessionMutationRequest mutationRequest;
        mutationRequest.actorRef = actorRef;
        mutationRequest.accountId = accountId;
        mutationRequest.sessionId = sessionId;
        mutationRequest.expectedResourceRevision =
            expectedResourceRevision;
        mutationRequest.requestId = requestId;
        mutationRequest.correlationId = correlationId;

        const PublicAccountSessionMutationResult mutated =
            mutation(mutationRequest);

        switch (mutated.status)
        {
            case PublicAccountSessionAdministrationStatus::ok:
                if (mutated.resource.accountId != accountId ||
                    mutated.resource.actorId.empty() ||
                    mutated.resource.session.sessionId != sessionId ||
                    mutated.resource.session.deviceId.empty() ||
                    mutated.resource.session.issuedFromCredentialId.empty() ||
                    mutated.resource.session.createdAt.empty() ||
                    !publicSessionLifecycleRevision(
                        mutated.resource.resourceRevision))
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                response =
                    publicAccountSessionResourceResponse(
                        mutated.resource,
                        path,
                        requestId,
                        correlationId,
                        "");
                return true;

            case PublicAccountSessionAdministrationStatus::invalid:
                response = problemResponse(
                    422,
                    "validation_error",
                    "Validation failed",
                    "The Account Session revoke request is invalid.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountSessionAdministrationStatus::notFound:
                response = notFoundProblem(
                    path, requestId, correlationId);
                return true;

            case PublicAccountSessionAdministrationStatus::revisionConflict:
                response = problemResponse(
                    412,
                    "revision_conflict",
                    "Resource revision conflict",
                    "The Account Session lifecycle changed after it was read.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountSessionAdministrationStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (publicAccountCredentialPath(path, accountId) ||
        publicAccountSessionPath(path, accountId))
    {
        response = methodNotAllowedProblem(
            path, requestId, correlationId, "GET");
        return true;
    }

    std::string rotationDeviceId;
    std::string rotationPreviousId;
    if (publicDeviceCredentialRotationPath(
            path, rotationDeviceId, rotationPreviousId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(path, requestId, correlationId);
            return true;
        }
        if (requestTarget != path || !idempotencyKey.empty())
        {
            response = invalidRequestProblem(
                path, "Rotation takes no query or Idempotency-Key.",
                requestId, correlationId);
            return true;
        }
        if (!applicationJsonContentType(contentType))
        {
            response = problemResponse(
                415, "invalid_request", "Unsupported media type",
                "Expected application/json.", path, requestId, correlationId);
            return true;
        }
        if (body.size() > 4096U || !emptyJsonObject(body))
        {
            response = problemResponse(
                422, "validation_error", "Validation failed",
                "Rotation requires an empty JSON object.",
                path, requestId, correlationId);
            return true;
        }
        if (ifMatch.empty())
        {
            response = problemResponse(
                428, "precondition_required", "Precondition required",
                "Strong If-Match required.", path, requestId, correlationId);
            return true;
        }
        std::string revision;
        if (!vdrsuite::http::publicStrongEntityTagResourceRevision(
                ifMatch, revision) ||
            revision != "device-credential-lifecycle:" +
                        rotationPreviousId + ":active")
        {
            response = invalidRequestProblem(
                path, "If-Match must identify the active credential.",
                requestId, correlationId);
            return true;
        }
        DeviceCredentialRotation callback;
        {
            std::lock_guard<std::mutex> lock(deviceCredentialRotationMutex_);
            callback = deviceCredentialRotation_;
        }
        if (!callback)
        {
            response = serviceUnavailableProblem(path, requestId, correlationId);
            return true;
        }
        PublicDeviceCredentialRotationRequest input;
        input.actorRef = actorRef;
        input.deviceId = rotationDeviceId;
        input.credentialId = rotationPreviousId;
        input.expectedResourceRevision = revision;
        input.requestId = requestId;
        input.correlationId = correlationId;
        auto result = callback(input);
        switch (result.status)
        {
            case PublicDeviceCredentialRotationStatus::rotated:
                if (result.deviceId != rotationDeviceId ||
                    result.actorId.empty() || result.credentialId.empty() ||
                    result.credentialSecret.empty() ||
                    result.credentialId == rotationPreviousId)
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                // Secret appears only in this POST issuance response, never
                // in ordinary lifecycle metadata, URLs, or audit records.
                response = jsonResponse(
                    "{\"deviceId\":\"" + jsonEscape(result.deviceId) +
                    "\",\"actorId\":\"" + jsonEscape(result.actorId) +
                    "\",\"credentialId\":\"" + jsonEscape(result.credentialId) +
                    "\",\"credentialSecret\":\"" +
                    jsonEscape(result.credentialSecret) + "\"}",
                    requestId, correlationId);
                response.headers["Cache-Control"] = "no-store";
                response.headers["Pragma"] = "no-cache";
                return true;
            case PublicDeviceCredentialRotationStatus::invalid:
                response = problemResponse(
                    422, "validation_error", "Validation failed",
                    "Invalid rotation request.", path, requestId, correlationId);
                return true;
            case PublicDeviceCredentialRotationStatus::notFound:
                response = notFoundProblem(path, requestId, correlationId);
                return true;
            case PublicDeviceCredentialRotationStatus::stateConflict:
            case PublicDeviceCredentialRotationStatus::revisionConflict:
                response = problemResponse(
                    412, "revision_conflict", "Resource revision conflict",
                    "Previous credential is no longer active.",
                    path, requestId, correlationId);
                return true;
            case PublicDeviceCredentialRotationStatus::unavailable:
                response = serviceUnavailableProblem(path, requestId, correlationId);
                return true;
        }
    }

    std::string lifecycleDeviceId;
    std::string lifecycleCredentialId;
    if (publicDeviceLifecyclePath(
            path, lifecycleDeviceId, lifecycleCredentialId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(path, requestId, correlationId);
            return true;
        }
        if (requestTarget != path || !idempotencyKey.empty())
        {
            response = invalidRequestProblem(
                path, "Lifecycle mutation takes no query or Idempotency-Key.",
                requestId, correlationId);
            return true;
        }
        if (!applicationJsonContentType(contentType))
        {
            response = problemResponse(
                415, "invalid_request", "Unsupported media type",
                "Expected application/json.", path, requestId, correlationId);
            return true;
        }
        if (body.size() > 4096U)
        {
            response = invalidRequestProblem(
                path, "Device lifecycle body too large.",
                requestId, correlationId);
            return true;
        }
        JsonSyntaxValidator validator(body);
        if (!validator.valid())
        {
            response = invalidRequestProblem(
                path, "Invalid Device lifecycle JSON.",
                requestId, correlationId);
            return true;
        }
        if (!emptyJsonObject(body))
        {
            response = problemResponse(
                422, "validation_error", "Validation failed",
                "Device lifecycle revoke requires an empty JSON object.",
                path, requestId, correlationId);
            return true;
        }
        if (ifMatch.empty())
        {
            response = problemResponse(
                428, "precondition_required", "Precondition required",
                "Strong If-Match required.", path, requestId, correlationId);
            return true;
        }
        std::string revision;
        if (!vdrsuite::http::publicStrongEntityTagResourceRevision(
                ifMatch, revision))
        {
            response = invalidRequestProblem(
                path, "Invalid lifecycle If-Match.",
                requestId, correlationId);
            return true;
        }
        const std::string prefix = lifecycleCredentialId.empty()
            ? "device-lifecycle:" + lifecycleDeviceId + ":"
            : "device-credential-lifecycle:" + lifecycleCredentialId + ":";
        if (revision != prefix + "active" &&
            revision != prefix + "revoked")
        {
            response = invalidRequestProblem(
                path, "If-Match does not identify this lifecycle resource.",
                requestId, correlationId);
            return true;
        }

        DeviceLifecycleMutation mutation;
        {
            std::lock_guard<std::mutex> lock(deviceLifecycleMutationMutex_);
            mutation = deviceLifecycleMutation_;
        }
        if (!mutation)
        {
            response = serviceUnavailableProblem(path, requestId, correlationId);
            return true;
        }
        PublicDeviceLifecycleMutationRequest input;
        input.actorRef = actorRef;
        input.deviceId = lifecycleDeviceId;
        input.credentialId = lifecycleCredentialId;
        input.expectedResourceRevision = revision;
        input.requestId = requestId;
        input.correlationId = correlationId;
        const PublicDeviceLifecycleResult result = mutation(input);
        switch (result.status)
        {
            case PublicDeviceLifecycleStatus::ok:
                if (result.resource.deviceId != lifecycleDeviceId ||
                    result.resource.credentialId != lifecycleCredentialId ||
                    result.resource.actorId.empty() ||
                    result.resource.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                response = publicDeviceLifecycleResponse(
                    result.resource, path, requestId, correlationId, "");
                return true;
            case PublicDeviceLifecycleStatus::invalid:
                response = problemResponse(
                    422, "validation_error", "Validation failed",
                    "Invalid lifecycle request.", path, requestId,
                    correlationId);
                return true;
            case PublicDeviceLifecycleStatus::notFound:
                response = notFoundProblem(path, requestId, correlationId);
                return true;
            case PublicDeviceLifecycleStatus::revisionConflict:
                response = problemResponse(
                    412, "revision_conflict", "Resource revision conflict",
                    "Device lifecycle changed since read.", path,
                    requestId, correlationId);
                return true;
            case PublicDeviceLifecycleStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    std::string deviceGrantDeviceId;
    if (publicDeviceGrantPath(path, deviceGrantDeviceId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(path, requestId, correlationId);
            return true;
        }
        if (requestTarget != path || !idempotencyKey.empty())
        {
            response = invalidRequestProblem(
                path, "Device Grant mutation takes no query or Idempotency-Key.",
                requestId, correlationId);
            return true;
        }
        if (!applicationJsonContentType(contentType))
        {
            response = problemResponse(415, "invalid_request",
                "Unsupported media type", "Expected application/json.",
                path, requestId, correlationId);
            return true;
        }
        if (body.size() > 4096U)
        {
            response = invalidRequestProblem(
                path, "Device Grant mutation body too large.",
                requestId, correlationId);
            return true;
        }
        JsonSyntaxValidator validator(body);
        if (!validator.valid())
        {
            response = invalidRequestProblem(
                path, "Invalid Device Grant JSON.",
                requestId, correlationId);
            return true;
        }
        std::string permission;
        std::string backendId;
        bool active = false;
        if (!parsePublicAccountGrantMutationBody(
                body, permission, backendId, active))
        {
            response = problemResponse(422, "validation_error",
                "Validation failed",
                "Expected exactly permission, backendId and active.",
                path, requestId, correlationId);
            return true;
        }
        if (ifMatch.empty())
        {
            response = problemResponse(428, "precondition_required",
                "Precondition required", "Strong If-Match required.",
                path, requestId, correlationId);
            return true;
        }
        std::string revision;
        if (!vdrsuite::http::publicStrongEntityTagResourceRevision(
                ifMatch, revision) || !publicGrantSetRevision(revision))
        {
            response = invalidRequestProblem(
                path, "Invalid Device Grant-set If-Match.",
                requestId, correlationId);
            return true;
        }
        DeviceGrantMutation mutation;
        {
            std::lock_guard<std::mutex> lock(deviceGrantMutationMutex_);
            mutation = deviceGrantMutation_;
        }
        if (!mutation)
        {
            response = serviceUnavailableProblem(path, requestId, correlationId);
            return true;
        }
        PublicDeviceGrantMutationRequest mutationRequest;
        mutationRequest.actorRef = actorRef;
        mutationRequest.deviceId = deviceGrantDeviceId;
        mutationRequest.expectedResourceRevision = revision;
        mutationRequest.permission = permission;
        mutationRequest.backendId = backendId;
        mutationRequest.active = active;
        mutationRequest.requestId = requestId;
        mutationRequest.correlationId = correlationId;
        const PublicDeviceGrantMutationResult result = mutation(mutationRequest);
        switch (result.status)
        {
            case PublicDeviceGrantStatus::ok:
                if (result.grantSet.deviceId != deviceGrantDeviceId ||
                    result.grantSet.actorId.empty() ||
                    !publicGrantSetRevision(result.grantSet.resourceRevision))
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                response = publicDeviceGrantSetResponse(
                    result.grantSet, path, requestId, correlationId, "");
                return true;
            case PublicDeviceGrantStatus::invalid:
                response = problemResponse(422, "validation_error",
                    "Validation failed",
                    "Unsupported Device Grant tuple or invalid request.",
                    path, requestId, correlationId);
                return true;
            case PublicDeviceGrantStatus::notFound:
                response = notFoundProblem(path, requestId, correlationId);
                return true;
            case PublicDeviceGrantStatus::revisionConflict:
                response = problemResponse(412, "revision_conflict",
                    "Resource revision conflict",
                    "Device Grant set changed since read.",
                    path, requestId, correlationId);
                return true;
            case PublicDeviceGrantStatus::unavailable:
                response = serviceUnavailableProblem(
                    path, requestId, correlationId);
                return true;
        }
    }

    if (publicAccountGrantPath(path, accountId))
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
                "Account Grant mutation does not accept query parameters.",
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
                "Account Grant mutation requires Content-Type application/json.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (body.size() > 4096U)
        {
            response = invalidRequestProblem(
                path,
                "The Account Grant mutation request body is too large.",
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
                "The Account Grant mutation request body is not valid JSON.",
                path,
                requestId,
                correlationId);
            return true;
        }

        std::string permission;
        std::string backendId;
        bool active = false;
        if (!parsePublicAccountGrantMutationBody(
                body,
                permission,
                backendId,
                active))
        {
            response = problemResponse(
                422,
                "validation_error",
                "Validation failed",
                "Account Grant mutation requires exactly permission, backendId and active.",
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
                "Account Grant mutation requires one strong If-Match entity tag.",
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

        if (!publicGrantSetRevision(expectedResourceRevision))
        {
            response = invalidRequestProblem(
                path,
                "If-Match does not identify an Account Grant-set revision.",
                requestId,
                correlationId);
            return true;
        }

        AccountGrantMutation mutation;
        {
            std::lock_guard<std::mutex> lock(
                accountGrantMutationMutex_);
            mutation = accountGrantMutation_;
        }
        if (!mutation)
        {
            response = serviceUnavailableProblem(
                path, requestId, correlationId);
            return true;
        }

        PublicAccountGrantMutationRequest mutationRequest;
        mutationRequest.actorRef = actorRef;
        mutationRequest.accountId = accountId;
        mutationRequest.expectedResourceRevision =
            expectedResourceRevision;
        mutationRequest.permission = permission;
        mutationRequest.backendId = backendId;
        mutationRequest.active = active;
        mutationRequest.requestId = requestId;
        mutationRequest.correlationId = correlationId;

        const PublicAccountGrantMutationResult mutated =
            mutation(mutationRequest);

        switch (mutated.status)
        {
            case PublicAccountGrantStatus::ok:
                if (mutated.grantSet.accountId != accountId ||
                    mutated.grantSet.actorId.empty() ||
                    !publicGrantSetRevision(
                        mutated.grantSet.resourceRevision))
                {
                    response = serviceUnavailableProblem(
                        path, requestId, correlationId);
                    return true;
                }
                response = publicAccountGrantSetResponse(
                    mutated.grantSet,
                    path,
                    requestId,
                    correlationId,
                    "");
                return true;

            case PublicAccountGrantStatus::invalid:
                response = problemResponse(
                    422,
                    "validation_error",
                    "Validation failed",
                    "The requested Account Grant tuple is not supported.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountGrantStatus::notFound:
                response = notFoundProblem(
                    path, requestId, correlationId);
                return true;

            case PublicAccountGrantStatus::revisionConflict:
                response = problemResponse(
                    412,
                    "revision_conflict",
                    "Resource revision conflict",
                    "The Account Grant set changed after it was read.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountGrantStatus::finalAdministrator:
                response = problemResponse(
                    409,
                    "operation_conflict",
                    "Operation conflict",
                    "The final usable administrator cannot lose role.admin@*.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicAccountGrantStatus::unavailable:
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
    std::string credentialId;
    std::string sessionId;
    std::string pairingRequestId;

    if (path == PublicDevicePairingCollectionPath)
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId,
            "GET, POST");
        return true;
    }

    if (publicDevicePairingPath(
            path,
            pairingRequestId))
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId,
            "GET, POST");
        return true;
    }

    if (publicAccountCredentialItemPath(
            path, accountId, credentialId))
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId,
            "GET, POST");
        return true;
    }

    if (publicAccountSessionItemPath(
            path, accountId, sessionId))
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId,
            "GET, POST");
        return true;
    }

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

    if (publicAccountCredentialPath(path, accountId) ||
        publicAccountSessionPath(path, accountId))
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId,
            "GET");
        return true;
    }

    std::string lifecycleDeviceId;
    std::string lifecycleCredentialId;
    if (publicDeviceLifecyclePath(
            path, lifecycleDeviceId, lifecycleCredentialId))
    {
        response = methodNotAllowedProblem(
            path, requestId, correlationId, "GET, POST");
        return true;
    }

    std::string deviceGrantDeviceId;
    if (publicDeviceGrantPath(path, deviceGrantDeviceId))
    {
        response = methodNotAllowedProblem(
            path, requestId, correlationId, "GET, POST");
        return true;
    }

    if (publicAccountGrantPath(path, accountId))
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
