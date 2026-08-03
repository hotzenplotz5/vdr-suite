#include "RuntimeConfig.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <string>

namespace
{

std::string environmentOrDefault(
    const char* name,
    const std::string& fallback)
{
    const char* value = std::getenv(name);

    if (value == nullptr)
    {
        return fallback;
    }

    std::string text(value);

    if (text.empty())
    {
        return fallback;
    }

    return text;
}

std::string environmentOrEmpty(
    const char* name)
{
    const char* value = std::getenv(name);
    return value == nullptr
        ? std::string()
        : std::string(value);
}

std::string lowercase(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

bool environmentBoolean(
    const char* name,
    const bool fallback)
{
    const char* value = std::getenv(name);

    if (value == nullptr || *value == '\0')
    {
        return fallback;
    }

    const std::string normalized = lowercase(value);

    if (normalized == "1" ||
        normalized == "true" ||
        normalized == "yes" ||
        normalized == "on")
    {
        return true;
    }

    if (normalized == "0" ||
        normalized == "false" ||
        normalized == "no" ||
        normalized == "off")
    {
        return false;
    }

    return fallback;
}

int environmentInteger(
    const char* name,
    const int fallback,
    const int minimum,
    const int maximum)
{
    const char* value = std::getenv(name);

    if (value == nullptr || *value == '\0')
    {
        return fallback;
    }

    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);

    if (errno != 0 ||
        end == value ||
        end == nullptr ||
        *end != '\0' ||
        parsed < minimum ||
        parsed > maximum)
    {
        return fallback;
    }

    return static_cast<int>(parsed);
}

bool isValidBackendId(
    const std::string& backendId)
{
    if (backendId.empty() || backendId.size() > 128)
    {
        return false;
    }

    for (const unsigned char character : backendId)
    {
        if (!std::isalnum(character) &&
            character != '-' &&
            character != '_' &&
            character != '.')
        {
            return false;
        }
    }

    return true;
}

std::map<std::string, std::string> parseArtworkRoots(
    const std::string& value)
{
    std::map<std::string, std::string> roots;

    if (value.empty())
    {
        return roots;
    }

    std::size_t start = 0;

    while (start <= value.size())
    {
        const std::size_t end = value.find(';', start);
        const std::string entry = value.substr(
            start,
            end == std::string::npos
                ? std::string::npos
                : end - start);
        const std::size_t separator = entry.find('=');

        if (entry.empty() ||
            separator == std::string::npos ||
            separator == 0 ||
            separator + 1 >= entry.size())
        {
            return {};
        }

        const std::string backendId =
            entry.substr(0, separator);
        const std::string root =
            entry.substr(separator + 1);

        if (!isValidBackendId(backendId) ||
            !std::filesystem::path(root).is_absolute() ||
            !roots.emplace(backendId, root).second)
        {
            return {};
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    return roots;
}

RuntimeSuiteBridgeConfig parseSuiteBridgeConfig()
{
    RuntimeSuiteBridgeConfig value;

    value.enabled = environmentBoolean(
        "VDR_SUITE_SUITE_BRIDGE_ENABLED",
        value.enabled);

    const std::string backendId = environmentOrDefault(
        "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID",
        value.backendId);
    value.backendId = isValidBackendId(backendId)
        ? backendId
        : "default";

    const std::string host = environmentOrDefault(
        "VDR_SUITE_SUITE_BRIDGE_HOST",
        value.host);
    value.host = host.size() <= 255
        ? host
        : "127.0.0.1";

    value.port = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_PORT",
        value.port,
        1,
        65535);
    value.connectTimeoutMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS",
        value.connectTimeoutMs,
        1,
        300000);
    value.ioTimeoutMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS",
        value.ioTimeoutMs,
        1,
        300000);
    value.operationTimeoutMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS",
        value.operationTimeoutMs,
        1,
        900000);
    value.pollIntervalMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS",
        value.pollIntervalMs,
        1,
        3600000);
    value.staleAfterMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS",
        value.staleAfterMs,
        1,
        3600000);
    value.offlineAfterMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS",
        value.offlineAfterMs,
        1,
        3600000);
    value.reconnectInitialMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS",
        value.reconnectInitialMs,
        1,
        3600000);
    value.reconnectMaximumMs = environmentInteger(
        "VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS",
        value.reconnectMaximumMs,
        1,
        3600000);

    value.staleAfterMs = std::max(
        value.staleAfterMs,
        value.pollIntervalMs);
    value.offlineAfterMs = std::max(
        value.offlineAfterMs,
        value.staleAfterMs);
    value.reconnectMaximumMs = std::max(
        value.reconnectMaximumMs,
        value.reconnectInitialMs);

    return value;
}

RuntimeSeriesArtworkFallbackConfig parseSeriesArtworkFallbackConfig()
{
    RuntimeSeriesArtworkFallbackConfig value;
    value.enabled = environmentBoolean(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED",
        value.enabled);
    return value;
}

}

RuntimeConfig::RuntimeConfig()
    : databasePath_(environmentOrDefault(
          "VDR_SUITE_DATABASE_PATH",
          "/tmp/vdr-suite-test.db")),
      vdrMode_("restfulapi"),
      vdrHost_("127.0.0.1"),
      vdrPort_(8002),
      httpListenHost_("0.0.0.0"),
      httpListenPort_(18080),
      recordingArtworkRoots_(parseArtworkRoots(
          environmentOrEmpty(
              "VDR_SUITE_RECORDING_ARTWORK_ROOTS"))),
      suiteBridge_(parseSuiteBridgeConfig()),
      seriesArtworkFallback_(parseSeriesArtworkFallbackConfig())
{
}

const std::string& RuntimeConfig::databasePath() const
{
    return databasePath_;
}

const std::string& RuntimeConfig::vdrMode() const
{
    return vdrMode_;
}

const std::string& RuntimeConfig::vdrHost() const
{
    return vdrHost_;
}

int RuntimeConfig::vdrPort() const
{
    return vdrPort_;
}

const std::string& RuntimeConfig::httpListenHost() const
{
    return httpListenHost_;
}

int RuntimeConfig::httpListenPort() const
{
    return httpListenPort_;
}

const std::map<std::string, std::string>& RuntimeConfig::recordingArtworkRoots() const
{
    return recordingArtworkRoots_;
}

const RuntimeSuiteBridgeConfig& RuntimeConfig::suiteBridge() const
{
    return suiteBridge_;
}

const RuntimeSeriesArtworkFallbackConfig& RuntimeConfig::seriesArtworkFallback() const
{
    return seriesArtworkFallback_;
}
