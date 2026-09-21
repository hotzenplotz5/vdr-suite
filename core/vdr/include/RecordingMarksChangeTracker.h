#pragma once

#include <cstdint>
#include <string>

class RecordingMarksChangeTracker final
{
public:
    bool observe(
        const std::string& counterEpoch,
        const std::uint64_t marksModified,
        const bool snapshotCurrent,
        const bool counterOverflow)
    {
        if (counterEpoch.empty())
        {
            return false;
        }

        if (counterOverflow)
        {
            blockedEpoch_ = counterEpoch;
            return false;
        }

        if (!snapshotCurrent)
        {
            return false;
        }

        if (!blockedEpoch_.empty())
        {
            if (counterEpoch == blockedEpoch_)
            {
                return false;
            }

            blockedEpoch_.clear();
        }

        if (!initialized_ || counterEpoch != counterEpoch_)
        {
            initialized_ = true;
            counterEpoch_ = counterEpoch;
            consumedMarksModified_ = marksModified;
            return false;
        }

        if (marksModified <= consumedMarksModified_)
        {
            return false;
        }

        consumedMarksModified_ = marksModified;
        return true;
    }

private:
    bool initialized_ = false;
    std::string counterEpoch_;
    std::string blockedEpoch_;
    std::uint64_t consumedMarksModified_ = 0;
};
