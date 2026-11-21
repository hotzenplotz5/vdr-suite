#include "TimerAssignmentDesiredNativeTimerSpecification.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::timers
{
namespace
{
constexpr const char* kPrefix =
    "timer-assignment-native-specification/1|";

void appendField(std::string& output, const std::string& value)
{
    output += std::to_string(value.size());
    output += ':';
    output += value;
    output += '|';
}

void appendField(std::string& output, std::int32_t value)
{
    appendField(output, std::to_string(value));
}

void appendField(std::string& output, bool value)
{
    appendField(output, std::string(value ? "1" : "0"));
}

bool readField(
    const std::string& encoded,
    std::size_t& position,
    std::string& value)
{
    if (position >= encoded.size()) return false;
    const std::size_t colon = encoded.find(':', position);
    if (colon == std::string::npos || colon == position) return false;

    std::size_t length = 0;
    for (std::size_t index = position; index < colon; ++index)
    {
        const char character = encoded[index];
        if (character < '0' || character > '9') return false;
        const std::size_t digit =
            static_cast<std::size_t>(character - '0');
        if (length >
            (std::numeric_limits<std::size_t>::max() - digit) / 10)
        {
            return false;
        }
        length = length * 10 + digit;
    }

    position = colon + 1;
    if (length > encoded.size() - position) return false;
    value.assign(encoded, position, length);
    position += length;
    if (position >= encoded.size() || encoded[position] != '|')
        return false;
    ++position;
    return true;
}

bool parseBoundedInt(
    const std::string& value,
    std::int32_t minimum,
    std::int32_t maximum,
    std::int32_t& parsed)
{
    if (value.empty()) return false;
    std::int64_t accumulator = 0;
    for (char character : value)
    {
        if (character < '0' || character > '9') return false;
        accumulator = accumulator * 10 + (character - '0');
        if (accumulator > maximum) return false;
    }
    if (accumulator < minimum || accumulator > maximum) return false;
    parsed = static_cast<std::int32_t>(accumulator);
    return true;
}

bool parseBool(const std::string& value, bool& parsed)
{
    if (value == "0")
    {
        parsed = false;
        return true;
    }
    if (value == "1")
    {
        parsed = true;
        return true;
    }
    return false;
}
} // namespace

std::string serializeTimerAssignmentDesiredNativeTimerSpecification(
    const NativeTimerSpecification& specification)
{
    if (!nativeTimerSpecificationValid(specification)) return {};

    std::string output = kPrefix;
    appendField(output, specification.channelId);
    appendField(output, specification.title);
    appendField(output, specification.directory);
    appendField(output, specification.day);
    appendField(output, specification.weekdays);
    appendField(output, specification.startTime);
    appendField(output, specification.endTime);
    appendField(output, specification.priority);
    appendField(output, specification.lifetime);
    appendField(output, specification.enabled);
    appendField(output, specification.vps);
    return output;
}

bool parseTimerAssignmentDesiredNativeTimerSpecification(
    const std::string& encoded,
    NativeTimerSpecification& specification)
{
    specification = {};
    const std::string prefix = kPrefix;
    if (encoded.compare(0, prefix.size(), prefix) != 0) return false;

    std::size_t position = prefix.size();
    std::string priority;
    std::string lifetime;
    std::string enabled;
    std::string vps;

    if (!readField(encoded, position, specification.channelId)
        || !readField(encoded, position, specification.title)
        || !readField(encoded, position, specification.directory)
        || !readField(encoded, position, specification.day)
        || !readField(encoded, position, specification.weekdays)
        || !readField(encoded, position, specification.startTime)
        || !readField(encoded, position, specification.endTime)
        || !readField(encoded, position, priority)
        || !readField(encoded, position, lifetime)
        || !readField(encoded, position, enabled)
        || !readField(encoded, position, vps)
        || position != encoded.size()
        || !parseBoundedInt(priority, 0, 99, specification.priority)
        || !parseBoundedInt(lifetime, 0, 99, specification.lifetime)
        || !parseBool(enabled, specification.enabled)
        || !parseBool(vps, specification.vps)
        || !nativeTimerSpecificationValid(specification))
    {
        specification = {};
        return false;
    }

    return serializeTimerAssignmentDesiredNativeTimerSpecification(specification)
        == encoded;
}

bool timerAssignmentDesiredNativeTimerSpecificationEquivalent(
    const NativeTimerSpecification& left,
    const NativeTimerSpecification& right)
{
    return left.channelId == right.channelId
        && left.title == right.title
        && left.directory == right.directory
        && left.day == right.day
        && left.weekdays == right.weekdays
        && left.startTime == right.startTime
        && left.endTime == right.endTime
        && left.priority == right.priority
        && left.lifetime == right.lifetime
        && left.enabled == right.enabled
        && left.vps == right.vps;
}

} // namespace vdrsuite::timers
