#include "EpgArtworkPathPolicy.h"

#include <filesystem>

namespace
{
bool isPathWithinRoot(
    const std::filesystem::path& path,
    const std::filesystem::path& root)
{
    auto pathIterator = path.begin();
    auto rootIterator = root.begin();

    for (; rootIterator != root.end(); ++rootIterator, ++pathIterator)
    {
        if (pathIterator == path.end() || *pathIterator != *rootIterator)
        {
            return false;
        }
    }

    return true;
}
}

std::vector<std::string> EpgArtworkPathPolicy::defaultAllowedRoots()
{
    return {
        "/var/cache/vdr/plugins/tvscraper",
        "/var/cache/vdr-suite/epg-artwork"
    };
}

bool EpgArtworkPathPolicy::resolveAllowedPath(
    const std::string& candidate,
    const std::vector<std::string>& allowedRoots,
    std::string& resolvedPath)
{
    resolvedPath.clear();

    std::error_code error;
    const std::filesystem::path canonicalCandidate =
        std::filesystem::weakly_canonical(candidate, error);

    if (error || canonicalCandidate.empty() || !canonicalCandidate.is_absolute())
    {
        return false;
    }

    for (const std::string& configuredRoot : allowedRoots)
    {
        error.clear();
        const std::filesystem::path canonicalRoot =
            std::filesystem::weakly_canonical(configuredRoot, error);

        if (!error &&
            !canonicalRoot.empty() &&
            canonicalRoot.is_absolute() &&
            isPathWithinRoot(canonicalCandidate, canonicalRoot))
        {
            resolvedPath = canonicalCandidate.string();
            return true;
        }
    }

    return false;
}

bool EpgArtworkPathPolicy::isAllowedPath(
    const std::string& candidate,
    const std::vector<std::string>& allowedRoots)
{
    std::string resolvedPath;
    return resolveAllowedPath(candidate, allowedRoots, resolvedPath);
}
