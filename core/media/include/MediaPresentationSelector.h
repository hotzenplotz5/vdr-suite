#pragma once

#include "MediaCapabilities.h"

class MediaPresentationSelector
{
public:
    MediaPresentationProfile select(
        const MediaSourceDescriptor& source,
        const ClientMediaCapabilities& client) const;
};
