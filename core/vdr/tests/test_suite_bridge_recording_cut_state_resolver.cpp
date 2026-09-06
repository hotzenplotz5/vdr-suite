#include "SuiteBridgeRecordingCutStateResolver.h"

#include <cassert>
#include <string>

namespace
{
class FakeTransport final : public ISuiteBridgeRecordingCutStateTransport
{
public:
    SuiteBridgeRecordingCutStateCommandReply reply;
    std::string requestedKey;

    SuiteBridgeRecordingCutStateCommandReply requestRecordingCutState(
        const std::string& recordingKey) override
    {
        requestedKey = recordingKey;
        return reply;
    }
};

std::string readyPayload(const std::string& key, const std::string& edited)
{
    return "vdr-suite-rcut-state/1 " + key +
        " 1 ready 1 1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa 1 4 2 0 0 " +
        edited + " 0 0";
}
}

int main()
{
    const std::string key = "c94d0eb9958a85079f81f059a436003c";
    const std::string edited = "0123456789abcdef0123456789abcdef";

    {
        FakeTransport transport;
        transport.reply = {true, 250, readyPayload(key, edited)};
        SuiteBridgeRecordingCutStateResolver resolver(transport);
        const auto state = resolver.resolve(key);
        assert(transport.requestedKey == key);
        assert(state.availability == VdrRecordingNativeCutStateAvailability::Available);
        assert(state.found);
        assert(state.ready);
        assert(state.reason == "ready");
        assert(state.marksRevision == "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        assert(state.markCount == 4);
        assert(state.sequenceCount == 2);
        assert(state.editedRecordingKey == edited);
        assert(!state.editedDestinationExists);
        assert(!state.editedRecordingFound);
    }

    {
        const auto state = SuiteBridgeRecordingCutStateResolver::parseReply(
            key,
            {true, 250,
             "vdr-suite-rcut-state/1 " + key +
             " 1 recording-handler-busy 0 1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa 1 4 2 0 4 " +
             edited + " 0 0"});
        assert(state.availability == VdrRecordingNativeCutStateAvailability::Available);
        assert(!state.ready);
        assert(state.handlerUsage == 4);
        assert(!state.editedRecordingFound);
    }

    {
        const auto state = SuiteBridgeRecordingCutStateResolver::parseReply(
            key,
            {true, 250,
             "vdr-suite-rcut-state/1 " + key +
             " 1 edited-destination-exists 0 1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa 1 4 2 0 0 " +
             edited + " 1 1"});
        assert(state.availability == VdrRecordingNativeCutStateAvailability::Available);
        assert(!state.ready);
        assert(state.editedDestinationExists);
        assert(state.editedRecordingFound);
    }

    {
        const auto state = SuiteBridgeRecordingCutStateResolver::parseReply(
            key,
            {true, 250,
             "vdr-suite-rcut-state/1 " + key +
             " 0 recording-not-found 0 0 none 0 0 0 0 0 none 0 0"});
        assert(state.availability ==
            VdrRecordingNativeCutStateAvailability::RecordingNotFound);
        assert(!state.found);
    }

    {
        std::string invalid = readyPayload(key, edited);
        const std::string needle = " 0 0 " + edited + " 0 0";
        const auto offset = invalid.find(needle);
        assert(offset != std::string::npos);
        invalid.replace(offset, needle.size(), " 0 3 " + edited + " 0 0");
        const auto state = SuiteBridgeRecordingCutStateResolver::parseReply(
            key, {true, 250, invalid});
        assert(state.availability == VdrRecordingNativeCutStateAvailability::InvalidPayload);
    }

    {
        const auto state = SuiteBridgeRecordingCutStateResolver::parseReply(
            key,
            {true, 250,
             "vdr-suite-rcut-state/1 " + key +
             " 1 edited-destination-exists 0 1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa 1 4 2 0 0 " +
             edited + " 0 1"});
        assert(state.availability == VdrRecordingNativeCutStateAvailability::InvalidPayload);
    }

    {
        const auto state = SuiteBridgeRecordingCutStateResolver::parseReply(
            key, {false, 451, "Recording cut state unavailable"});
        assert(state.availability == VdrRecordingNativeCutStateAvailability::TransportError);
    }

    {
        FakeTransport transport;
        SuiteBridgeRecordingCutStateResolver resolver(transport);
        const auto state = resolver.resolve("/srv/vdr/video/test.rec");
        assert(state.availability == VdrRecordingNativeCutStateAvailability::InvalidPayload);
        assert(transport.requestedKey.empty());
    }

    return 0;
}
