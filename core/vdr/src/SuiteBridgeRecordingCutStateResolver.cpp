#include "SuiteBridgeRecordingCutStateResolver.h"

#include "VdrRecordingNativeIdentity.h"

#include <cerrno>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
class ParseError final : public std::runtime_error
{
public:
    using runtime_error::runtime_error;
};

bool lowerHex32(const std::string& value)
{
    if (value.size() != 32) return false;
    for (const unsigned char character : value)
    {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f')))
            return false;
    }
    return true;
}

bool flag(const std::string& value)
{
    if (value == "1") return true;
    if (value == "0") return false;
    throw ParseError("invalid boolean token");
}

int nonNegativeInt(const std::string& value)
{
    if (value.empty()) throw ParseError("empty integer token");
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (errno != 0 || end == nullptr || *end != '\0' || parsed < 0 ||
        parsed > std::numeric_limits<int>::max())
        throw ParseError("invalid integer token");
    return static_cast<int>(parsed);
}

std::vector<std::string> tokens(const std::string& payload)
{
    std::istringstream stream(payload);
    std::vector<std::string> values;
    std::string value;
    while (stream >> value)
    {
        values.push_back(value);
        if (values.size() > 15) throw ParseError("too many tokens");
    }
    if (values.size() != 15) throw ParseError("unexpected token count");
    return values;
}

void validateConsistency(VdrRecordingNativeCutState& state)
{
    if (!VdrRecordingNativeIdentity::isValidKey(state.recordingKey))
        throw ParseError("invalid recording identity");
    if (state.markCount < 0 || state.sequenceCount < 0 ||
        state.inUseFlags < 0 || state.handlerUsage < 0)
        throw ParseError("negative native fact");

    if (state.marksReadable)
    {
        if (!lowerHex32(state.marksRevision))
            throw ParseError("readable marks require revision");
    }
    else if (!state.marksRevision.empty())
    {
        throw ParseError("unreadable marks must not claim revision");
    }

    if (!state.editedRecordingKey.empty())
    {
        if (!VdrRecordingNativeIdentity::isValidKey(state.editedRecordingKey) ||
            state.editedRecordingKey == state.recordingKey)
            throw ParseError("invalid edited recording identity");
    }
    if ((state.editedDestinationExists || state.editedRecordingFound) &&
        state.editedRecordingKey.empty())
        throw ParseError("edited destination requires identity");
    if (state.editedRecordingFound && !state.editedDestinationExists)
        throw ParseError("edited result requires destination existence");

    if (state.ready)
    {
        if (!state.found || state.reason != "ready" || !state.marksReadable ||
            !state.marksFilePresent || state.markCount <= 0 ||
            state.sequenceCount <= 0 || state.inUseFlags != 0 ||
            state.handlerUsage != 0 || state.editedRecordingKey.empty() ||
            state.editedDestinationExists || state.editedRecordingFound)
            throw ParseError("inconsistent ready cut state");
    }
    else if (state.reason == "ready")
    {
        throw ParseError("ready reason requires ready state");
    }

    if (!state.found)
    {
        if (state.ready || state.marksReadable || state.marksFilePresent ||
            state.markCount != 0 || state.sequenceCount != 0 ||
            state.inUseFlags != 0 || state.handlerUsage != 0 ||
            !state.marksRevision.empty() || !state.editedRecordingKey.empty() ||
            state.editedDestinationExists || state.editedRecordingFound)
            throw ParseError("missing source carries native state");
        if (state.reason == "recording-not-found")
            state.availability =
                VdrRecordingNativeCutStateAvailability::RecordingNotFound;
        else if (state.reason == "recording-identity-ambiguous")
            state.availability =
                VdrRecordingNativeCutStateAvailability::IdentityAmbiguous;
        else
            throw ParseError("invalid missing-source reason");
        return;
    }

    state.availability = VdrRecordingNativeCutStateAvailability::Available;
}
}

SuiteBridgeRecordingCutStateResolver::SuiteBridgeRecordingCutStateResolver(
    ISuiteBridgeRecordingCutStateTransport& transport)
    : transport_(transport)
{
}

VdrRecordingNativeCutState SuiteBridgeRecordingCutStateResolver::resolve(
    const std::string& recordingKey)
{
    if (!VdrRecordingNativeIdentity::isValidKey(recordingKey))
    {
        VdrRecordingNativeCutState result;
        result.availability = VdrRecordingNativeCutStateAvailability::InvalidPayload;
        result.recordingKey = recordingKey;
        result.diagnostic = "invalid recording key";
        return result;
    }
    return parseReply(recordingKey, transport_.requestRecordingCutState(recordingKey));
}

VdrRecordingNativeCutState SuiteBridgeRecordingCutStateResolver::parseReply(
    const std::string& expectedRecordingKey,
    const SuiteBridgeRecordingCutStateCommandReply& reply)
{
    VdrRecordingNativeCutState result;
    result.recordingKey = expectedRecordingKey;
    if (!reply.transportSucceeded || reply.replyCode != 250)
    {
        result.availability = VdrRecordingNativeCutStateAvailability::TransportError;
        result.diagnostic = "RCUT transport unavailable";
        return result;
    }

    try
    {
        const auto value = tokens(reply.payload);
        if (value[0] != VdrRecordingNativeCutState::Protocol ||
            value[1] != expectedRecordingKey ||
            !VdrRecordingNativeIdentity::isValidKey(value[1]))
            throw ParseError("protocol or recording identity mismatch");

        result.recordingKey = value[1];
        result.found = flag(value[2]);
        result.reason = value[3];
        result.ready = flag(value[4]);
        result.marksReadable = flag(value[5]);
        result.marksRevision = value[6] == "none" ? std::string() : value[6];
        result.marksFilePresent = flag(value[7]);
        result.markCount = nonNegativeInt(value[8]);
        result.sequenceCount = nonNegativeInt(value[9]);
        result.inUseFlags = nonNegativeInt(value[10]);
        result.handlerUsage = nonNegativeInt(value[11]);
        result.editedRecordingKey = value[12] == "none"
            ? std::string() : value[12];
        result.editedDestinationExists = flag(value[13]);
        result.editedRecordingFound = flag(value[14]);
        if (result.reason.empty() || result.reason.size() > 96)
            throw ParseError("invalid reason token");
        validateConsistency(result);
        return result;
    }
    catch (const std::exception& error)
    {
        result = {};
        result.recordingKey = expectedRecordingKey;
        result.availability = VdrRecordingNativeCutStateAvailability::InvalidPayload;
        result.diagnostic = error.what();
        return result;
    }
}
