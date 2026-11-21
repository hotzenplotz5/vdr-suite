#include "EpgArtworkPathPolicy.h"

#include <filesystem>
#include <cstdlib>
#include <set>

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
    const char* configured = std::getenv("VDR_SUITE_EPG_ARTWORK_ROOTS");
    if (configured == nullptr || *configured == '\0')
    {
        return {
            "/var/cache/vdr/plugins/tvscraper",
            "/var/cache/vdr-suite/epg-artwork"
        };
    }

    // An operator-supplied list replaces rather than widens the defaults.
    // Malformed overrides deny all local artwork, never fall back to the
    // distribution-specific default whitelist.
    std::vector<std::string> roots;
    std::set<std::string> seen;
    const std::string value(configured);
    std::size_t start = 0;
    while (start <= value.size())
    {
        const std::size_t end = value.find(';', start);
        const std::string item = value.substr(
            start, end == std::string::npos
                ? std::string::npos
                : end - start);
        const std::filesystem::path path(item);
        if (item.empty() || item.size() > 4096U ||
            !path.is_absolute() || path == path.root_path() ||
            item.find_first_of("\r\n") != std::string::npos ||
            roots.size() >= 8U)
        {
            return {};
        }

        for (const auto& part : path)
        {
            if (part == "." || part == "..")
            {
                return {};
            }
        }

        const std::string root = path.lexically_normal().string();
        if (root == "/" || !seen.insert(root).second)
        {
            return {};
        }
        roots.push_back(root);
        if (end == std::string::npos)
        {
            break;
        }
        start = end + 1;
    }
    return roots;
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
