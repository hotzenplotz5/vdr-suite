#pragma once

#include "EpgArtworkReference.h"
#include "VdrEvent.h"

#include <string>

struct EpgArtworkResolution
{
    bool attempted = false;
    bool found = false;
    EpgArtworkReference artwork;
};

class IEpgArtworkResolver
{
public:
    virtual ~IEpgArtworkResolver() = default;

    virtual EpgArtworkResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) = 0;
};
