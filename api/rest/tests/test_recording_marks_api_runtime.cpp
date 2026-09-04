#include "RecordingMarksApiRuntime.h"

#include "VdrRecordingNativeIdentity.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
class FakeResolver final : public IVdrRecordingNativeMarksResolver
{
public:
    VdrRecordingNativeMarks resolve(
        const std::string& recordingKey) override
    {
        ++calls;
        lastKey = recordingKey;
        VdrRecordingNativeMarks result = next;
        if (result.recordingKey.empty()) result.recordingKey = recordingKey;
        return result;
    }

    int calls = 0;
    std::string lastKey;
    VdrRecordingNativeMarks next;
};

VdrRecording makeRecording(
    const std::string& id = "7")
{
    VdrRecording recording;
    recording.id = id;
    recording.backendId = "default";
    recording.backendNativeId =
        "/srv/vdr/video/Movies/Test/2026-09-04.07.00.1-0.rec";
    recording.title = "Test";
    return recording;
}

VdrRecordingNativeMarks availableMarks()
{
    VdrRecordingNativeMarks marks;
    marks.availability = VdrRecordingNativeMarksAvailability::Available;
    marks.schema = 1;
    marks.found = true;
    marks.reason = "none";
    marks.recordingIdentitySchema = 1;
    marks.state = "present";
    marks.framesPerSecond = 25.0;
    marks.isPesRecording = false;
    marks.inUseFlags = 2;
    marks.marksFilePresent = true;
    marks.sequenceCount = 1;
    marks.marksRevision = "0123456789abcdef0123456789abcdef";
    VdrRecordingNativeMark mark;
    mark.positionFrame = 250;
    mark.timecode = "0:00:10.00";
    mark.positionSeconds = 10.0;
    mark.comment = "begin";
    marks.marks.push_back(mark);
    return marks;
}
}

int main()
{
    RecordingMarksApiRuntime& runtime = RecordingMarksApiRuntime::instance();
    runtime.reset();

    ApiResponse response;
    assert(!runtime.tryHandleGet("/api/vdr/recordings/query", response));
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 503);

    FakeResolver resolver;
    resolver.next = availableMarks();
    std::vector<VdrRecording> recordings{makeRecording()};

    RecordingMarksBackendAvailability backendAvailability =
        RecordingMarksBackendAvailability::Available;
    int lookupCalls = 0;
    int backendCalls = 0;

    assert(runtime.configure(
        [&](const std::string& backendId) {
            ++lookupCalls;
            assert(backendId == "default");
            return recordings;
        },
        [&](const std::string& backendId) {
            ++backendCalls;
            assert(backendId == "default");
            RecordingMarksBackendAccess access;
            access.availability = backendAvailability;
            access.resolver =
                backendAvailability == RecordingMarksBackendAvailability::Available
                    ? &resolver
                    : nullptr;
            return access;
        }));
    assert(runtime.configured());

    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 200);
    assert(response.headers.at("Cache-Control") == "no-store");
    assert(response.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(response.body.find("\"recordingId\":\"7\"") != std::string::npos);
    assert(response.body.find("\"inUse\":true") != std::string::npos);
    assert(response.body.find("\"positionFrame\":250") != std::string::npos);
    assert(response.body.find("\"positionSeconds\":10") != std::string::npos);
    assert(response.body.find("0123456789abcdef0123456789abcdef") != std::string::npos);
    assert(response.body.find("/srv/vdr") == std::string::npos);
    assert(response.body.find("backendNativeId") == std::string::npos);
    assert(response.body.find("recordingKey") == std::string::npos);
    assert(resolver.calls == 1);
    assert(resolver.lastKey ==
        VdrRecordingNativeIdentity::keyForNativeId(
            recordings.front().backendNativeId));
    assert(lookupCalls == 1);
    assert(backendCalls == 1);

    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=%37",
        response));
    assert(response.statusCode == 200);
    assert(resolver.calls == 2);

    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default",
        response));
    assert(response.statusCode == 400);
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7&backendNativeId=%2Fsrv%2Fvdr",
        response));
    assert(response.statusCode == 400);
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7&path=%2Fsrv%2Fvdr",
        response));
    assert(response.statusCode == 400);
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7&recordingId=8",
        response));
    assert(response.statusCode == 400);

    recordings.clear();
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 404);
    assert(resolver.calls == 2);

    recordings = {makeRecording(), makeRecording()};
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 409);
    assert(resolver.calls == 2);

    recordings = {makeRecording()};
    backendAvailability = RecordingMarksBackendAvailability::CapabilityUnavailable;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 503);
    assert(resolver.calls == 2);

    backendAvailability = RecordingMarksBackendAvailability::BackendNotFound;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 404);

    backendAvailability = RecordingMarksBackendAvailability::Available;
    resolver.next = availableMarks();
    resolver.next.availability =
        VdrRecordingNativeMarksAvailability::RecordingNotFound;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 409);

    resolver.next = availableMarks();
    resolver.next.availability =
        VdrRecordingNativeMarksAvailability::NativeUnreadable;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 503);

    resolver.next = availableMarks();
    resolver.next.availability =
        VdrRecordingNativeMarksAvailability::TransportError;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 503);

    resolver.next = availableMarks();
    resolver.next.availability =
        VdrRecordingNativeMarksAvailability::InvalidPayload;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 502);

    resolver.next = availableMarks();
    resolver.next.recordingKey =
        "ffffffffffffffffffffffffffffffff";
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 502);

    runtime.reset();
    assert(!runtime.configured());
    return 0;
}
