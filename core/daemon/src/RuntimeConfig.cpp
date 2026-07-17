#include "RuntimeConfig.h"

#include <cstdlib>
#include <string>
#include <vector>

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

std::vector<std::string> splitArtworkRoots(
    const std::string& value)
{
    std::vector<std::string> roots;
    std::size_t start = 0;

    while (start <= value.size())
    {
        const std::size_t end = value.find(':', start);
        const std::string root = value.substr(
            start,
            end == std::string::npos
                ? std::string::npos
                : end - start);

        if (!root.empty())
        {
            roots.push_back(root);
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    if (roots.empty())
    {
        roots.push_back("/var/cache/vdr/plugins/tvscraper");
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
      recordingArtworkRoots_(splitArtworkRoots(
          environmentOrDefault(
              "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
              "/var/cache/vdr/plugins/tvscraper")))
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

const std::vector<std::string>& RuntimeConfig::recordingArtworkRoots() const
{
    return recordingArtworkRoots_;
}
