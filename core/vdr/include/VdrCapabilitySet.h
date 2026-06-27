#pragma once

class VdrCapabilitySet
{
public:
    bool snapshotRead = false;
    bool statusRead = false;
    bool healthRead = false;
    bool recordingsRead = false;
    bool timersRead = false;
    bool channelsRead = false;
    bool eventsRead = false;
    bool eventsSelectiveRead = false;
    bool epgSearchFuzzyFallback = false;
    bool epgSearchFuzzyNative = false;
    bool searchTimerPreviewNative = false;

    static VdrCapabilitySet snapshotReadOnly()
    {
        VdrCapabilitySet capabilities;

        capabilities.snapshotRead = true;
        capabilities.statusRead = true;
        capabilities.healthRead = true;
        capabilities.recordingsRead = true;
        capabilities.timersRead = true;
        capabilities.channelsRead = true;
        capabilities.eventsRead = true;
        capabilities.eventsSelectiveRead = true;
        capabilities.epgSearchFuzzyFallback = true;
        capabilities.epgSearchFuzzyNative = false;
        capabilities.searchTimerPreviewNative = false;

        return capabilities;
    }
};
