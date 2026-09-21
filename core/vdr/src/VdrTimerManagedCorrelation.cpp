#include "VdrTimerManagedCorrelation.h"

#include <cstddef>
#include <string>

namespace
{

constexpr std::size_t kMaxIdentityLength = 160;
const std::string kReservedPrefix = "<vdr-suite-managed-timer";
const std::string kMarkerPrefix =
    "<vdr-suite-managed-timer-v1 assignment=\"";
const std::string kBindingSeparator = "\" binding=\"";
const std::string kMarkerSuffix = "\"/>";

bool validIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

char hexDigit(unsigned int value)
{
    return static_cast<char>(value < 10 ? '0' + value : 'A' + value - 10);
}

std::string encodeHex(const std::string& value)
{
    std::string encoded;
    encoded.reserve(value.size() * 2);
    for (const unsigned char ch : value)
    {
        encoded.push_back(hexDigit((ch >> 4) & 0x0F));
        encoded.push_back(hexDigit(ch & 0x0F));
    }
    return encoded;
}

int hexValue(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
    if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
    return -1;
}

bool decodeHex(const std::string& encoded, std::string& value)
{
    if (encoded.empty()
        || encoded.size() % 2 != 0
        || encoded.size() > kMaxIdentityLength * 2)
    {
        return false;
    }

    value.clear();
    value.reserve(encoded.size() / 2);
    for (std::size_t i = 0; i < encoded.size(); i += 2)
    {
        const int high = hexValue(encoded[i]);
        const int low = hexValue(encoded[i + 1]);
        if (high < 0 || low < 0) return false;
        value.push_back(static_cast<char>((high << 4) | low));
    }
    return validIdentity(value);
}

bool sameCorrelation(
    const VdrTimerManagedCorrelation& left,
    const VdrTimerManagedCorrelation& right)
{
    return left.timerAssignmentId == right.timerAssignmentId
        && left.nativeTimerBindingId == right.nativeTimerBindingId;
}

std::string markerFor(const VdrTimerManagedCorrelation& correlation)
{
    return kMarkerPrefix
        + encodeHex(correlation.timerAssignmentId)
        + kBindingSeparator
        + encodeHex(correlation.nativeTimerBindingId)
        + kMarkerSuffix;
}

VdrTimerManagedCorrelationParseResult parseResult(
    VdrTimerManagedCorrelationStatus status)
{
    VdrTimerManagedCorrelationParseResult result;
    result.status = status;
    return result;
}

}

bool vdrTimerManagedCorrelationValid(
    const VdrTimerManagedCorrelation& correlation)
{
    return validIdentity(correlation.timerAssignmentId)
        && validIdentity(correlation.nativeTimerBindingId);
}

VdrTimerManagedCorrelationParseResult parseVdrTimerManagedCorrelation(
    const std::string& aux)
{
    const std::size_t reserved = aux.find(kReservedPrefix);
    if (reserved == std::string::npos)
        return parseResult(VdrTimerManagedCorrelationStatus::absent);

    if (aux.find(kReservedPrefix, reserved + 1) != std::string::npos)
        return parseResult(VdrTimerManagedCorrelationStatus::conflictingMarker);

    if (aux.compare(reserved, kMarkerPrefix.size(), kMarkerPrefix) != 0)
        return parseResult(VdrTimerManagedCorrelationStatus::malformedMarker);

    const std::size_t assignmentStart = reserved + kMarkerPrefix.size();
    const std::size_t separator = aux.find(kBindingSeparator, assignmentStart);
    if (separator == std::string::npos)
        return parseResult(VdrTimerManagedCorrelationStatus::malformedMarker);

    const std::size_t bindingStart = separator + kBindingSeparator.size();
    const std::size_t suffix = aux.find(kMarkerSuffix, bindingStart);
    if (suffix == std::string::npos)
        return parseResult(VdrTimerManagedCorrelationStatus::malformedMarker);

    VdrTimerManagedCorrelation correlation;
    if (!decodeHex(
            aux.substr(assignmentStart, separator - assignmentStart),
            correlation.timerAssignmentId)
        || !decodeHex(
            aux.substr(bindingStart, suffix - bindingStart),
            correlation.nativeTimerBindingId))
    {
        return parseResult(VdrTimerManagedCorrelationStatus::malformedMarker);
    }

    VdrTimerManagedCorrelationParseResult result;
    result.status = VdrTimerManagedCorrelationStatus::ok;
    result.correlation = std::move(correlation);
    return result;
}

VdrTimerManagedCorrelationAttachResult attachVdrTimerManagedCorrelation(
    const std::string& aux,
    const VdrTimerManagedCorrelation& correlation)
{
    VdrTimerManagedCorrelationAttachResult result;
    result.aux = aux;

    if (!vdrTimerManagedCorrelationValid(correlation))
    {
        result.status = VdrTimerManagedCorrelationStatus::invalidCorrelation;
        return result;
    }

    const auto parsed = parseVdrTimerManagedCorrelation(aux);
    if (parsed.status == VdrTimerManagedCorrelationStatus::absent)
    {
        result.aux += markerFor(correlation);
        result.status = VdrTimerManagedCorrelationStatus::ok;
        return result;
    }

    if (parsed.status == VdrTimerManagedCorrelationStatus::ok)
    {
        result.status = sameCorrelation(parsed.correlation, correlation)
            ? VdrTimerManagedCorrelationStatus::ok
            : VdrTimerManagedCorrelationStatus::conflictingMarker;
        return result;
    }

    result.status = parsed.status;
    return result;
}
