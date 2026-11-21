#pragma once

#include "NativeTimerBinding.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>

namespace vdrsuite::timers
{

struct NativeTimerSpecification
{
    std::string channelId;
    std::string title;
    std::string directory;
    std::string day;
    std::string weekdays = "-------";
    std::string startTime;
    std::string endTime;
    std::int32_t priority = 50;
    std::int32_t lifetime = 99;
    bool enabled = true;
    bool vps = false;
};

inline bool nativeTimerSpecificationValid(
    const NativeTimerSpecification& specification)
{
    constexpr std::size_t kMaxIdentityLength = 160;
    constexpr std::size_t kMaxTextLength = 1024;

    const auto bounded = [](const std::string& value, std::size_t maximum) {
        return value.size() <= maximum;
    };
    const auto nonEmptyBounded =
        [&](const std::string& value, std::size_t maximum) {
            return !value.empty() && bounded(value, maximum);
        };
    const auto validWeekdays = [](const std::string& value) {
        if (value.size() != 7) return false;
        for (const unsigned char character : value)
        {
            if (character != '-' && !std::isalpha(character)) return false;
        }
        return true;
    };
    const auto validHhmm = [](const std::string& value) {
        if (value.empty() || value.size() > 4) return false;
        for (const unsigned char character : value)
            if (!std::isdigit(character)) return false;

        std::string normalized(4 - value.size(), '0');
        normalized += value;
        const int hour =
            (normalized[0] - '0') * 10 + normalized[1] - '0';
        const int minute =
            (normalized[2] - '0') * 10 + normalized[3] - '0';
        return hour <= 23 && minute <= 59;
    };

    return nonEmptyBounded(specification.channelId, kMaxIdentityLength)
        && bounded(specification.title, kMaxTextLength)
        && bounded(specification.directory, kMaxTextLength)
        && bounded(specification.day, kMaxIdentityLength)
        && validWeekdays(specification.weekdays)
        && validHhmm(specification.startTime)
        && validHhmm(specification.endTime)
        && specification.priority >= 0
        && specification.priority <= 99
        && specification.lifetime >= 0
        && specification.lifetime <= 99;
}
std::string nativeTimerSpecificationFingerprint(
    const NativeTimerSpecification& specification);
bool nativeTimerObservationMatchesSpecification(
    const NativeTimerSpecification& specification,
    const NativeTimerObservedState& observedState);

}
