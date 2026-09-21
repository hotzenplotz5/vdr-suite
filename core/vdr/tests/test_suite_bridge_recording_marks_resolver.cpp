#include "SuiteBridgeRecordingMarksResolver.h"

#include <cassert>
#include <string>

namespace
{
class FakeTransport final : public ISuiteBridgeRecordingMarksTransport
{
public:
    SuiteBridgeRecordingMarksCommandReply reply;
    std::string requestedKey;

    SuiteBridgeRecordingMarksCommandReply requestRecordingMarks(
        const std::string& recordingKey) override
    {
        requestedKey = recordingKey;
        return reply;
    }
};

std::string readablePayload(const std::string& key)
{
    return
        "{\"schema\":1,\"found\":true,\"reason\":\"none\","
        "\"recordingIdentitySchema\":1,\"recordingKey\":\"" + key +
        "\",\"state\":\"present\",\"framesPerSecond\":25,"
        "\"isPesRecording\":false,\"inUseFlags\":0,"
        "\"marksFilePresent\":true,\"sequenceCount\":1,"
        "\"marksRevision\":\"0123456789abcdef0123456789abcdef\","
        "\"marks\":["
        "{\"positionFrame\":100,\"timecode\":\"00:00:04.00\","
        "\"positionSeconds\":4,\"comment\":\"begin\"},"
        "{\"positionFrame\":250,\"timecode\":\"00:00:10.00\","
        "\"positionSeconds\":10,\"comment\":\"end\"}]}";
}
}

int main()
{
    const std::string key = "c94d0eb9958a85079f81f059a436003c";

    {
        FakeTransport transport;
        transport.reply = {true, 250, readablePayload(key)};
        SuiteBridgeRecordingMarksResolver resolver(transport);
        const VdrRecordingNativeMarks marks = resolver.resolve(key);

        assert(transport.requestedKey == key);
        assert(marks.availability ==
            VdrRecordingNativeMarksAvailability::Available);
        assert(marks.found);
        assert(marks.state == "present");
        assert(marks.framesPerSecond == 25.0);
        assert(marks.sequenceCount == 1);
        assert(marks.marks.size() == 2);
        assert(marks.marks[0].positionFrame == 100);
        assert(marks.marks[0].positionSeconds == 4.0);
        assert(marks.marks[1].positionFrame == 250);
        assert(marks.marksRevision ==
            "0123456789abcdef0123456789abcdef");
    }

    {
        const std::string payload =
            "{\"schema\":1,\"found\":false,"
            "\"reason\":\"recording_not_found\","
            "\"recordingIdentitySchema\":1,\"recordingKey\":\"" + key +
            "\",\"state\":\"none\",\"framesPerSecond\":0,"
            "\"isPesRecording\":false,\"inUseFlags\":0,"
            "\"marksFilePresent\":false,\"sequenceCount\":0,"
            "\"marksRevision\":\"\",\"marks\":[]}";
        const VdrRecordingNativeMarks marks =
            SuiteBridgeRecordingMarksResolver::parseReply(
                key, {true, 250, payload});
        assert(marks.availability ==
            VdrRecordingNativeMarksAvailability::RecordingNotFound);
        assert(!marks.found);
    }

    {
        const std::string payload =
            "{\"schema\":1,\"found\":true,\"reason\":\"none\","
            "\"recordingIdentitySchema\":1,\"recordingKey\":\"" + key +
            "\",\"state\":\"unreadable\",\"framesPerSecond\":25,"
            "\"isPesRecording\":false,\"inUseFlags\":0,"
            "\"marksFilePresent\":true,\"sequenceCount\":0,"
            "\"marksRevision\":\"\",\"marks\":[]}";
        const VdrRecordingNativeMarks marks =
            SuiteBridgeRecordingMarksResolver::parseReply(
                key, {true, 250, payload});
        assert(marks.availability ==
            VdrRecordingNativeMarksAvailability::NativeUnreadable);
    }

    {
        std::string payload = readablePayload(key);
        const std::string needle = "\"positionSeconds\":4";
        const std::size_t offset = payload.find(needle);
        assert(offset != std::string::npos);
        payload.replace(offset, needle.size(), "\"positionSeconds\":5");
        const VdrRecordingNativeMarks marks =
            SuiteBridgeRecordingMarksResolver::parseReply(
                key, {true, 250, payload});
        assert(marks.availability ==
            VdrRecordingNativeMarksAvailability::InvalidPayload);
    }

    {
        const VdrRecordingNativeMarks marks =
            SuiteBridgeRecordingMarksResolver::parseReply(
                key,
                {false, 451, "Recording marks payload unavailable"});
        assert(marks.availability ==
            VdrRecordingNativeMarksAvailability::TransportError);
        assert(marks.diagnostic == "Recording marks payload unavailable");
    }

    {
        FakeTransport transport;
        SuiteBridgeRecordingMarksResolver resolver(transport);
        const VdrRecordingNativeMarks marks = resolver.resolve(
            "/srv/vdr/video/test.rec");
        assert(marks.availability ==
            VdrRecordingNativeMarksAvailability::InvalidPayload);
        assert(transport.requestedKey.empty());
    }

    return 0;
}
