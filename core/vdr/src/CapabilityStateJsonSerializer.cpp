#include "CapabilityStateJsonSerializer.h"

#include <sstream>

namespace
{
const char* boolToJson(
    bool value)
{
    return value ? "true" : "false";
}

const char* availabilityToJson(
    CapabilityAvailability availability)
{
    switch (availability)
    {
    case CapabilityAvailability::Available:
        return "available";
    case CapabilityAvailability::Unsupported:
        return "unsupported";
    case CapabilityAvailability::NotConfigured:
        return "notConfigured";
    case CapabilityAvailability::Offline:
        return "offline";
    case CapabilityAvailability::TemporarilyUnavailable:
        return "temporarilyUnavailable";
    }

    return "unknown";
}
}

std::string CapabilityStateJsonSerializer::serialize(
    const CapabilityState& state) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"capability\":\"" << state.capabilityName() << "\","
        << "\"supported\":" << boolToJson(state.supported()) << ","
        << "\"availability\":\"" << availabilityToJson(state.availability()) << "\","
        << "\"availableNow\":" << boolToJson(state.availableNow()) << ","
        << "\"reason\":\"" << state.reason() << "\""
        << "}";

    return json.str();
}
