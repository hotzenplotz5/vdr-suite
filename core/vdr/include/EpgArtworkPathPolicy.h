#pragma once

#include <string>
#include <vector>

class EpgArtworkPathPolicy
{
public:
    static std::vector<std::string> defaultAllowedRoots();

    static bool resolveAllowedPath(
        const std::string& candidate,
        const std::vector<std::string>& allowedRoots,
        std::string& resolvedPath);

    static bool isAllowedPath(
        const std::string& candidate,
        const std::vector<std::string>& allowedRoots);
};
