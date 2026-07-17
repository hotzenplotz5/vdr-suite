#include "RuntimeConfig.h"

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
              "VDR_SUITE_RECORDING_ARTWORK_ROOTS")))
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
