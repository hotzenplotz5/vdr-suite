#pragma once

#include "EpgArtworkReference.h"

#include <string>

class EpgArtworkPublicJsonSerializer
{
public:
    std::string serialize(
        const EpgArtworkReference& artwork) const;

    std::string artworkUrl(
        const EpgArtworkReference& artwork) const;

private:
    static std::string escapeJsonString(
        const std::string& value);

    static std::string percentEncodeQueryValue(
        const std::string& value);
};
