#pragma once

#include "MediaCapabilities.h"

class RecordingHlsResumeProfile
{
public:
    static MediaPresentationProfile prepare(
        const MediaPresentationProfile& profile,
        int startPositionSeconds);
};
