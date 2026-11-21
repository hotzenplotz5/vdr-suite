#include "NativeTimerSpecification.h"

#include <cstddef>
#include <string>

namespace vdrsuite::timers
{
namespace
{

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
