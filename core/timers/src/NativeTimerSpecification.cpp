#include "NativeTimerSpecification.h"

#include <cctype>
#include <cstddef>
#include <string>

namespace vdrsuite::timers
{
namespace
{

constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxTextLength = 1024;

bool bounded(const std::string& value, std::size_t maximum)
{
    return value.size() <= maximum;
}

bool nonEmptyBounded(const std::string& value, std::size_t maximum)
{
    return !value.empty() && bounded(value, maximum);
}

bool validWeekdays(const std::string& value)
{
    if (value.size() != 7) return false;
    for (const unsigned char ch : value)
    {
        if (ch != '-' && !std::isalpha(ch)) return false;
    }
    return true;
}

bool validHhmm(const std::string& value)
{
    if (value.empty() || value.size() > 4) return false;
    for (const unsigned char ch : value)
        if (!std::isdigit(ch)) return false;

    std::string normalized(4 - value.size(), '0');
    normalized += value;
    const int hour = (normalized[0] - '0') * 10 + normalized[1] - '0';
    const int minute = (normalized[2] - '0') * 10 + normalized[3] - '0';
    return hour <= 23 && minute <= 59;
}

std::string normalizedHhmm(const std::string& value)
{
    return std::string(4 - value.size(), '0') + value;
}

void append(std::string& output, const std::string& value)
{
    output += std::to_string(value.size());
    output += ':';
    output += value;
    output += '|';
}

void append(std::string& output, std::int32_t value)
{
    append(output, std::to_string(value));
}

void append(std::string& output, bool value)
{
    append(output, std::string(value ? "1" : "0"));
}

}

bool nativeTimerSpecificationValid(
    const NativeTimerSpecification& specification)
{
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
    const NativeTimerSpecification& specification)
{
    if (!nativeTimerSpecificationValid(specification)) return {};

    std::string fingerprint = "native-timer-specification/1|";
    append(fingerprint, specification.channelId);
    append(fingerprint, specification.title);
    append(fingerprint, specification.directory);
    append(fingerprint, specification.day);
    append(fingerprint, specification.weekdays);
    append(fingerprint, normalizedHhmm(specification.startTime));
    append(fingerprint, normalizedHhmm(specification.endTime));
    append(fingerprint, specification.priority);
    append(fingerprint, specification.lifetime);
    append(fingerprint, specification.enabled);
    append(fingerprint, specification.vps);
    return fingerprint;
}

bool nativeTimerObservationMatchesSpecification(
    const NativeTimerSpecification& specification,
    const NativeTimerObservedState& observedState)
{
    if (!nativeTimerSpecificationValid(specification)
        || !nativeTimerObservedStateValid(observedState))
    {
        return false;
    }

    return specification.channelId == observedState.channelId
        && specification.title == observedState.title
        && specification.directory == observedState.directory
        && specification.day == observedState.day
        && specification.weekdays == observedState.weekdays
        && normalizedHhmm(specification.startTime)
            == normalizedHhmm(observedState.startTime)
        && normalizedHhmm(specification.endTime)
            == normalizedHhmm(observedState.endTime)
        && specification.priority == observedState.priority
        && specification.lifetime == observedState.lifetime
        && specification.enabled == observedState.enabled
        && specification.vps == observedState.vps;
}

}
